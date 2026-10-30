#ifndef NGEN_BMI_UTILITIES_HPP
#define NGEN_BMI_UTILITIES_HPP

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <boost/type_index.hpp>
#include "bmi/Bmi_Adapter.hpp"
#include "utilities/bmi_primitive_type.hpp"

/*
 * Typed access to BMI model variables.
 *
 * A BMI adapter reports a variable in three ways the standard does not tie together: a type, as a
 * string; a size, as a count of bytes; and a buffer or pointer carrying no type at all. Reading or
 * writing one therefore means choosing a C++ type, sizing a buffer, and trusting the two agree.
 *
 * A type name is resolved once, to a primitive_type (see bmi_primitive_type.hpp), and that is what
 * is passed around after.
 *
 * Prefer the helpers below to calling Bmi_Adapter::GetValue() or GetValuePtr() and casting the
 * result:
 *
 *   with_bmi_type()       resolve a type name to a C++ type, by parse_bmi_type() and with_primitive_type()
 *   GetValue<T>()         read a variable, or one item of it, converted to T
 *   GetValuePtr<T>()      borrow the model's own storage, typed
 *   get_value_as_type()   build a value in the variable's declared type
 *   get_values_as_type()  build a buffer in the variable's declared type
 *   require_item_size()   confirm a type name and a reported item size describe the same width
 *   resolve_var_type()    the type name to address a model's variable as, checked against its item size
 *
 * The raw adapter calls stay public -- the adapters implement them, and a caller may genuinely want
 * bytes -- but they are not the ordinary way to read or write a variable.
 */

namespace models {
    namespace bmi {
        /**
         * @brief Call @p f with the C++ type that a BMI type name denotes.
         *
         * Shorthand for with_primitive_type(parse_bmi_type(type), f), for callers holding a name.
         *
         * @param type The BMI type name to resolve.
         * @param f A callable templated on the type to use, as for with_primitive_type().
         * @return Whatever @p f returns.
         * @throw unknown_bmi_type If no C++ type corresponds to @p type.
         */
        template <typename F>
        auto with_bmi_type(const std::string &type, F &&f)
        {
            return with_primitive_type(parse_bmi_type(type), std::forward<F>(f));
        }

        /**
         * @brief Throw unless @p type is exactly @p item_size bytes.
         *
         * A model reports a variable's type name and the size of one item separately, and nothing in
         * BMI makes the two agree. Every buffer here is sized from one and addressed as the other, so a
         * disagreement reads or writes past the end -- on a read, out of the buffer; on a write, out of
         * it by whatever the model takes. Neither is detectable afterwards, so refuse up front.
         *
         * @param type The type the items are to be addressed as.
         * @param item_size The size in bytes of one item, as the model reports it.
         * @param described What is being resolved, for the message; e.g. "variable 'X' of model 'Y'".
         * @param spelled How to name @p type in the message; e.g. the name the model reported for it.
         * @throw std::runtime_error If @p type is not @p item_size bytes.
         */
        inline void require_item_size(primitive_type type, size_t item_size, const std::string &described,
                                      const std::string &spelled)
        {
            const size_t declared = size_of(type);
            if (declared != item_size) {
                throw std::runtime_error("Cannot use " + described + ": type '" + spelled + "' is " +
                                         std::to_string(declared) + " bytes, but the model reports an item size of " +
                                         std::to_string(item_size) + " bytes.");
            }
        }

        /** @brief As above, naming @p type by its enumerator in the message. */
        inline void require_item_size(primitive_type type, size_t item_size, const std::string &described)
        {
            require_item_size(type, item_size, described, to_string(type));
        }

        /**
         * @brief As above, for a type given by name.
         *
         * @throw unknown_bmi_type If @p type has no C++ type behind it.
         */
        inline void require_item_size(const std::string &type, size_t item_size, const std::string &described)
        {
            require_item_size(parse_bmi_type(type), item_size, described, type);
        }

        /**
         * @brief Resolve the type to address a model's variable as, checked against its item size.
         *
         * The model's own type name is reduced by its adapter's get_analogous_cxx_type(), which may need
         * the item size to settle it, and the result must then describe that item size exactly.
         *
         * @param model The adapter holding the variable.
         * @param name The name of the variable.
         * @return The variable's primitive_type, which is as wide as its items.
         * @throw unknown_bmi_type If no C++ type corresponds to the resolved name.
         * @throw std::runtime_error If that type is not as wide as the model says the items are.
         */
        inline primitive_type resolve_var_type(Bmi_Adapter &model, const std::string &name)
        {
            const size_t item_size = static_cast<size_t>(model.GetVarItemsize(name));
            const std::string reported = model.GetVarType(name);
            const primitive_type type = parse_bmi_type(model.get_analogous_cxx_type(reported, item_size));
            require_item_size(type, item_size, "variable '" + name + "' of model '" + model.get_model_name() + "'",
                              reported);
            return type;
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
            std::transform(begin, end, ptr.get(), [](auto const& in){ return static_cast<T>(in); });
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
        std::shared_ptr<void> get_values_as_type(primitive_type type, Iterator begin, Iterator end)
        {
            return with_primitive_type(type, [&]<typename T>() -> std::shared_ptr<void> {
                return as_c_array<T>(begin, end);
            });
        }

        /** @brief As above, for a type given by name. */
        template <typename Iterator>
        std::shared_ptr<void> get_values_as_type(const std::string &type, Iterator begin, Iterator end)
        {
            return get_values_as_type(parse_bmi_type(type), begin, end);
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
        std::shared_ptr<void> get_value_as_type(primitive_type type, T value)
        {
            return with_primitive_type(type, [&]<typename U>() -> std::shared_ptr<void> {
                return std::make_shared<U>(static_cast<U>(value));
            });
        }

        /** @brief As above, for a type given by name. */
        template<typename T>
        std::shared_ptr<void> get_value_as_type(const std::string &type, T value)
        {
            return get_value_as_type(parse_bmi_type(type), value);
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
            //Determine what type we need to cast from, before the model writes into a buffer sized by it
            primitive_type type;
            try {
                type = resolve_var_type(model, name);
            } catch (const unknown_bmi_type& e) {
                // The shared table knows the type name, but not what was being read or as what.
                throw std::runtime_error("Unable to get value of variable " + name + " of model " +
                                         model.get_model_name() + " as " +
                                         boost::typeindex::type_id<T>().pretty_name() +
                                         ": no logic for converting variable type " + e.type_name);
            }

            //C++ form of malloc
            void* data = ::operator new(total_mem); //Possible to allocate 0 bytes...
            // Use smart pointer to ensure cleanup on throw/out of scope...
            // This works, and is relatively cheap since the lambda is stateless, only one instance should be created.
            auto sptr = std::shared_ptr<void>(data, [](void *p) { ::operator delete(p); });
            //Delegate to specific adapter's GetValue()
            //Note, may be able to optimize this furthur using GetValuePtr
            //which would avoid copying in the BMI model and copying again here
            model.GetValue(name, data);

            return with_primitive_type(type, [&]<typename S>() -> std::vector<T> {
                return helper::make_vector<T>(static_cast<const S*>(data), num_items);
            });
        }

        /**
         * @brief Read one item of a variable from a BMI model adapter, converted to @tparam T.
         *
         * The single-item counterpart of the GetValue() above, for callers wanting one value of a
         * variable that may hold many: the model is asked for that item alone, through its adapter's
         * GetValueAtIndices(), rather than for a copy of the whole variable.
         *
         * @tparam T Type to convert the item to
         * @param model Bmi model adapter holding the variable
         * @param name Bmi variable name to query the model for
         * @param index Index of the item within the variable
         * @return T The item of variable @p name at @p index
         * @throw std::out_of_range If @p index is outside the variable.
         */
        template <typename T>
        T GetValue(Bmi_Adapter& model, const std::string& name, int index) {
            const int item_size = model.GetVarItemsize(name);
            const int num_items = item_size > 0 ? model.GetVarNbytes(name) / item_size : 0;
            if (index < 0 || index >= num_items) {
                throw std::out_of_range("Cannot get item " + std::to_string(index) + " of variable " + name +
                                        " of model " + model.get_model_name() + ", which has " +
                                        std::to_string(num_items) + " items.");
            }

            primitive_type type;
            try {
                type = resolve_var_type(model, name);
            } catch (const unknown_bmi_type& e) {
                throw std::runtime_error("Unable to get value of variable " + name + " of model " +
                                         model.get_model_name() + " as " +
                                         boost::typeindex::type_id<T>().pretty_name() +
                                         ": no logic for converting variable type " + e.type_name);
            }

            return with_primitive_type(type, [&]<typename S>() -> T {
                S item;
                model.GetValueAtIndices(name, &item, &index, 1);
                return static_cast<T>(item);
            });
        }
    }
}

#endif //NGEN_BMI_UTILITIES_HPP
