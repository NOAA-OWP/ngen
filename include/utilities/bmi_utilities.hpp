#ifndef NGEN_BMI_UTILITIES_HPP
#define NGEN_BMI_UTILITIES_HPP

#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <boost/type_index.hpp>
#include "bmi/Bmi_Adapter.hpp"

/*
 * Typed access to BMI model variables.
 *
 * A BMI adapter reports a variable in three ways the standard does not tie together: a type, as a
 * string; a size, as a count of bytes; and a buffer or pointer carrying no type at all. Reading or
 * writing one therefore means choosing a C++ type, sizing a buffer, and trusting the two agree.
 *
 * Prefer the helpers below to calling Bmi_Adapter::GetValue() or GetValuePtr() and casting the
 * result:
 *
 *   with_bmi_type()       resolve a type name to a C++ type; the one table the others are built on
 *   GetValue<T>()         read a variable, converted to T
 *   GetValuePtr<T>()      borrow the model's own storage, typed
 *   get_value_as_type()   build a value in the variable's declared type
 *   get_values_as_type()  build a buffer in the variable's declared type
 *   require_item_size()   confirm a type name and a reported item size describe the same width
 *
 * The raw adapter calls stay public -- the adapters implement them, and a caller may genuinely want
 * bytes -- but they are not the ordinary way to read or write a variable.
 */

namespace models {
    namespace bmi {
        /** @brief Thrown when a BMI type name has no C++ type behind it. */
        struct unknown_bmi_type : std::runtime_error {
            explicit unknown_bmi_type(const std::string &type_name)
                : std::runtime_error("No C++ type corresponds to the BMI type name '" + type_name + "'")
                , type_name(type_name) {}

            std::string type_name;
        };

        /**
         * @brief Call @p f with the C++ type that a BMI type name denotes.
         *
         * BMI reports a variable's type as a string, and every operation on that variable has to pick the
         * matching C++ type before it can do anything. This is the one place that mapping lives: @p f is a
         * callable with a template parameter, invoked as `f.template operator()<T>()`, and this picks the T.
         *
         * Several spellings may denote one type -- "real" and "float", "integer" and "int" -- and the
         * table resolves them alike. Which spelling a given model reports is not this function's concern.
         *
         * @code
         * // "Give me this value as whatever the variable declares." The target type is a string, so the
         * // cast cannot be written directly: capture the value, and let the table supply the type.
         * std::shared_ptr<void> as_declared_type(const std::string &type, double value)
         * {
         *     return with_bmi_type(type, [&]<typename T>() -> std::shared_ptr<void> {
         *         return std::make_shared<T>(static_cast<T>(value));
         *     });
         * }
         * @endcode
         *
         * Two things there are easy to get wrong.
         *
         * The lambda takes no arguments. `value` arrives by capture, which leaves the parameter list free
         * for the one thing this function supplies -- `f.template operator()<T>()` passes a type and
         * nothing else. That is what lets one table serve callers whose data is a scalar, an iterator
         * pair, or a pointer and a count: each seals its own data into the closure.
         *
         * The `-> std::shared_ptr<void>` is load-bearing. `make_shared<T>` yields a `shared_ptr<T>`, a
         * different type in every branch, and the branches have to agree on one return type. Converting
         * there is the only place T is still known. Omitting it is a compile error rather than a silent
         * conversion -- as is a body that does not compile for every type below, even one no caller ever
         * asks for, since all of them are instantiated.
         *
         * @param type The BMI type name to resolve.
         * @param f A callable templated on the type to use.
         * @return Whatever @p f returns.
         * @throw unknown_bmi_type If no C++ type corresponds to @p type.
         */
        template <typename F>
        auto with_bmi_type(const std::string &type, F &&f)
        {
            if (type == "double" || type == "double precision")
                return f.template operator()<double>();

            if (type == "long double")
                return f.template operator()<long double>();

            if (type == "float" || type == "real")
                return f.template operator()<float>();

            // The stdint spellings, by typedef rather than by hand: int64_t is long on some platforms
            // and long long on others, and the typedef is right on both.
            if (type == "int8" || type == "int8_t")
                return f.template operator()<std::int8_t>();

            if (type == "uint8" || type == "uint8_t")
                return f.template operator()<std::uint8_t>();

            if (type == "int16" || type == "int16_t")
                return f.template operator()<std::int16_t>();

            if (type == "uint16" || type == "uint16_t")
                return f.template operator()<std::uint16_t>();

            if (type == "int32" || type == "int32_t")
                return f.template operator()<std::int32_t>();

            if (type == "uint32" || type == "uint32_t")
                return f.template operator()<std::uint32_t>();

            if (type == "int64" || type == "int64_t")
                return f.template operator()<std::int64_t>();

            if (type == "uint64" || type == "uint64_t")
                return f.template operator()<std::uint64_t>();

            if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
                return f.template operator()<short>();

            if (type == "unsigned short" || type == "unsigned short int")
                return f.template operator()<unsigned short>();

            if (type == "int" || type == "signed" || type == "signed int" || type == "integer")
                return f.template operator()<int>();

            if (type == "unsigned" || type == "unsigned int")
                return f.template operator()<unsigned int>();

            if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
                return f.template operator()<long>();

            if (type == "unsigned long" || type == "unsigned long int")
                return f.template operator()<unsigned long>();

            if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
                return f.template operator()<long long>();

            if (type == "unsigned long long" || type == "unsigned long long int")
                return f.template operator()<unsigned long long>();

            throw unknown_bmi_type(type);
        }

        /**
         * @brief Throw unless @p type names a C++ type of exactly @p item_size bytes.
         *
         * A model reports a variable's type name and the size of one item separately, and nothing in
         * BMI makes the two agree. Every buffer here is sized from one and addressed as the other, so a
         * disagreement reads or writes past the end -- on a read, out of the buffer; on a write, out of
         * it by whatever the model takes. Neither is detectable afterwards, so refuse up front.
         *
         * @param type A BMI type name.
         * @param item_size The size in bytes of one item, as the model reports it.
         * @param described What is being resolved, for the message; e.g. "variable 'X' of model 'Y'".
         * @throw unknown_bmi_type If @p type has no C++ type behind it.
         * @throw std::runtime_error If that type is not @p item_size bytes.
         */
        inline void require_item_size(const std::string &type, size_t item_size, const std::string &described)
        {
            const size_t declared = with_bmi_type(type, []<typename T>() -> size_t { return sizeof(T); });
            if (declared != item_size) {
                throw std::runtime_error("Cannot use " + described + ": type '" + type + "' is " +
                                         std::to_string(declared) + " bytes, but the model reports an item size of " +
                                         std::to_string(item_size) + " bytes.");
            }
        }

        namespace helper {

            /**
             * @brief make a vector of type @tparam TO of size @param count. Copy elements of @tparam FROM type in data into the vector.
             * 
             * This function relies on the std library to cast data approriately during the construction of the vector.
             * 
             * @tparam TO data type to cast elements of the data pointer to
             * @tparam FROM data type of the pointer data is extracted from
             * @param data pointer to @param count number of @tparam FROM type elements
             * @param count number of elements in the @param data pointer
             * @return std::vector<TO> vector filled with @param count number of elements extracted from @param data
             */
            template<typename TO, typename FROM>
            static inline std::vector<TO> make_vector(const FROM* data, const size_t& count){
                //Let Return Value Optimization (move semantics) help here (no copy of vector)
                return std::vector<TO>(data, data+count);
            }

        }

        /**
         * @brief Template function for copying iterator range into contiguous array.
         *
         * This function will iterate the range and cast the iterator value to type T
         * and copy that value into a C-like array of contiguous, dynamically allocated memory.
         * This array is stored in a smart pointer with a custom array deleter.
         *
         * @tparam T
         * @tparam Iterator
         * @param begin
         * @param end
         * @return std::shared_ptr<T>
         */
        template <typename T, typename Iterator>
        std::shared_ptr<T> as_c_array(Iterator begin, Iterator end){
            //Make a shared pointer large enough to hold all elements
            //This is a CONTIGUOUS array of type T
            //Must provide a custom deleter to delete the array
            std::shared_ptr<T> ptr( new T[std::distance(begin, end)], [](T *p) { delete[] p; } );
            Iterator it = begin;
            int i = 0;
            while(it != end){
                //Be safe and cast the input to the desired type
                ptr.get()[i] = static_cast<T>(*it);
                ++it;
                ++i;
            }
            return ptr;
        }

        /**
         * @brief Gets values in iterator range, casted based on @p type then returned as typeless (void) pointer.
         *
         * @tparam Iterator
         * @param type
         * @param begin
         * @param end
         * @return std::shared_ptr<void>
         */
        template <typename Iterator>
        std::shared_ptr<void> get_values_as_type(std::string type, Iterator begin, Iterator end)
        {
            return with_bmi_type(type, [&]<typename T>() -> std::shared_ptr<void> {
                return as_c_array<T>(begin, end);
            });
        }

        /**
         * @brief Gets a single value, casted based on @p type then returned as a typeless (void) pointer.
         *
         * @tparam T
         * @param type
         * @param value
         * @return std::shared_ptr<void>
         */
        template<typename T>
        std::shared_ptr<void> get_value_as_type(std::string type, T value)
        {
            return with_bmi_type(type, [&]<typename U>() -> std::shared_ptr<void> {
                return std::make_shared<U>(static_cast<U>(value));
            });
        }

        /**
         * @brief Get a pointer to a model variable's own storage, addressed as @tparam T.
         *
         * The sibling of GetValue() for callers that want the model's memory rather than a copy of it.
         * Bmi_Adapter exposes the pointer untyped, because BMI has no way to describe what is behind it;
         * every caller therefore has to supply the type, and nothing in BMI obliges that type to be the
         * width the model allocated. Indexing with the wrong stride walks off the variable, so the item
         * size is checked here rather than at each call site.
         *
         * An adapter whose language cannot hand out pointers says so from GetValuePtr() itself; this
         * does not need to know which those are.
         *
         * @tparam T The type to address the variable's storage as.
         * @param model The adapter holding the variable.
         * @param name The name of the variable.
         * @return A pointer to the model's own storage, which the model continues to own.
         * @throw std::runtime_error If the model's items are not @tparam T wide, or it cannot give a pointer.
         */
        template <typename T>
        T *GetValuePtr(Bmi_Adapter &model, const std::string &name)
        {
            const size_t item_size = static_cast<size_t>(model.GetVarItemsize(name));
            if (item_size != sizeof(T)) {
                throw std::runtime_error("Cannot get pointer to variable " + name + " of model " +
                                         model.get_model_name() + " as a " + std::to_string(sizeof(T)) +
                                         " byte type; its items are " + std::to_string(item_size) + " bytes.");
            }
            return static_cast<T *>(model.GetValuePtr(name));
        }

        /**
         * @brief Copy values from a BMI model adapter and box them into a vector.
         * 
         * @tparam T Type to cast data from the BMI model to
         * @tparam A Bmi model type
         * @param model Bmi model adapter that interfaces to @tparam A bmi
         * @param name Bmi variable name to query the model for
         * @return std::vector<T> Copy of data from the BMI model for variable @param name
         */
        template <typename T>
        std::vector<T> GetValue(Bmi_Adapter& model, const std::string& name) {
            //TODO make model const ref
            int total_mem = model.GetVarNbytes(name);
            int item_size = model.GetVarItemsize(name);
            if( total_mem == 0 || item_size == 0){
                 // Early stop if no items to be returned (also prevents possible divide by 0 computing num_items)
                 // Becuase this function is used by others which assume the return value to have AT LEAST
                 // 1 value, then this is a terminal error.  This happens when the BMI model hasn't properly
                 // initialized/allocated/set the value to something meaningful.  This can happen, for example,
                 // with grid data if the variables themselves are dynamically allocated based on grid properties
                 // but the grid is 0, then we essentially get a sentinal pointer with no data in it.
                 // one possible way to handle this would be to ensure a single element vector with NaN were returned here
                 // but since this function has to deal with both int and floating point types, that isn't possible
                 // because int has not NaN representation.  So the best we can do at this point is raise a runtime exception
                throw std::runtime_error("Unable to get value of variable "+name+". Model "+ model.get_model_name() + 
                                         " reports no valid items (Nbytes = "+std::to_string(total_mem) +
                                         ", Itemsize = "+std::to_string(item_size)+".");
            }
            int num_items = total_mem/item_size;
            //Determine what type we need to cast from
            std::string type = model.get_analogous_cxx_type(model.GetVarType(name), item_size);
            
            //C++ form of malloc
            void* data = ::operator new(total_mem); //Possible to allocate 0 bytes...
            // Use smart pointer to ensure cleanup on throw/out of scope...
            // This works, and is relatively cheap since the lambda is stateless, only one instance should be created.
            auto sptr = std::shared_ptr<void>(data, [](void *p) { ::operator delete(p); });
            //Delegate to specific adapter's GetValue()
            //Note, may be able to optimize this furthur using GetValuePtr
            //which would avoid copying in the BMI model and copying again here
            model.GetValue(name, data);

            try {
                require_item_size(type, static_cast<size_t>(item_size),
                                  "variable '" + name + "' of model '" + model.get_model_name() + "'");
                return with_bmi_type(type, [&]<typename S>() -> std::vector<T> {
                    return helper::make_vector<T>(static_cast<const S*>(data), num_items);
                });
            } catch (const unknown_bmi_type&) {
                // The shared table knows the type name, but not what was being read or as what.
                throw std::runtime_error("Unable to get value of variable " + name +
                                " as " + boost::typeindex::type_id<T>().pretty_name() + ": no logic for converting variable type " + type);
            }
        }
    }
}

#endif //NGEN_BMI_UTILITIES_HPP
