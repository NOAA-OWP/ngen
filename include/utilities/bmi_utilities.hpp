#ifndef NGEN_BMI_UTILITIES_HPP
#define NGEN_BMI_UTILITIES_HPP

#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <boost/type_index.hpp>
#include "Bmi_Adapter.hpp"

namespace models {
    namespace bmi {
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
            //Use std::vector range constructor to ensure contiguous storage of values
            //Return the pointer to the contiguous storage
            if (type == "double" || type == "double precision")
                return as_c_array<double>(begin, end);

            if (type == "float" || type == "real")
                return as_c_array<float>(begin, end);

            if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
                return as_c_array<short>(begin, end);

            if (type == "unsigned short" || type == "unsigned short int")
                return as_c_array<unsigned short>(begin, end);

            if (type == "int" || type == "signed" || type == "signed int" || type == "integer")
                return as_c_array<int>(begin, end);

            if (type == "unsigned" || type == "unsigned int")
                return as_c_array<unsigned int>(begin, end);

            if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
                return as_c_array<long>(begin, end);

            if (type == "unsigned long" || type == "unsigned long int")
                return as_c_array<unsigned long>(begin, end);

            if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
                return as_c_array<long long>(begin, end);

            if (type == "unsigned long long" || type == "unsigned long long int")
                return as_c_array<unsigned long long>(begin, end);

            throw std::runtime_error("Unable to get values of iterable as type" + type +
                " : no logic for converting values to variable's type.");
        }

        // TODO: need to modify this to support arrays properly, since in general that's what BMI modules deal with
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
            if (type == "double" || type == "double precision")
                return std::make_shared<double>( static_cast<double>(value) );

            if (type == "float" || type == "real")
                return std::make_shared<float>( static_cast<float>(value) );

            if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int")
                return std::make_shared<short>( static_cast<short>(value) );

            if (type == "unsigned short" || type == "unsigned short int")
                return std::make_shared<unsigned short>( static_cast<unsigned short>(value) );

            if (type == "int" || type == "signed" || type == "signed int" || type == "integer")
                return std::make_shared<int>( static_cast<int>(value) );

            if (type == "unsigned" || type == "unsigned int")
                return std::make_shared<unsigned int>( static_cast<unsigned int>(value) );

            if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int")
                return std::make_shared<long>( static_cast<long>(value) );

            if (type == "unsigned long" || type == "unsigned long int")
                return std::make_shared<unsigned long>( static_cast<unsigned long>(value) );

            if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int")
                return std::make_shared<long long>( static_cast<long long>(value) );

            if (type == "unsigned long long" || type == "unsigned long long int")
                return std::make_shared<unsigned long long>( static_cast<unsigned long long>(value) );

            throw std::runtime_error("Unable to get value of variable as type '" + type +
                "': no logic for converting value to variable's type.");
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
            std::vector<T> result;

            /*
            * Allows the std::vector constructor to type cast the values as it copies them.
            * I don't see any other way around the typing issue other than an explicit copy of each...
            * Now there is an early optimization that allows types that align to pass through uncopied
            * but that also only works if GetValuePtr returns a compatible pointer that can iterate
            * on the recieving side correctly.  This may be tricky for certain langague adapters (Fortran?)
            * Untill this becomes burdensome on memory/time, I suggest copying and converting each value
            */

            if (type == "long double"){
                result = helper::make_vector<T>( (long double*) data, num_items);
            }
            else if (type == "double"){
                result = helper::make_vector<T>( (double*) data, num_items);
            }
            else if (type == "float"){
                result = helper::make_vector<T>( (float*) data, num_items);
            }
            else if (type == "short" || type == "short int" || type == "signed short" || type == "signed short int"){
                result = helper::make_vector<T>( (short*) data, num_items);
            }
            else if (type == "unsigned short" || type == "unsigned short int"){
                result = helper::make_vector<T>( (unsigned short*) data, num_items);
            }
            else if (type == "int" || type == "signed" || type == "signed int"){
                result = helper::make_vector<T>( (int*) data, num_items);
            }
            else if (type == "unsigned" || type == "unsigned int"){
                result = helper::make_vector<T>( (unsigned int*) data, num_items);
            }
            else if (type == "long" || type == "long int" || type == "signed long" || type == "signed long int"){
                result = helper::make_vector<T>( (long*) data, num_items);
            }
            else if (type == "unsigned long" || type == "unsigned long int"){
                result = helper::make_vector<T>( (unsigned long*) data, num_items);
            }
            else if (type == "long long" || type == "long long int" || type == "signed long long" || type == "signed long long int"){
                result = helper::make_vector<T>( (long long*) data, num_items);
            }
            else if (type == "unsigned long long" || type == "unsigned long long int"){
                result = helper::make_vector<T>( (unsigned long long*) data, num_items);
            }
            else{
                throw std::runtime_error("Unable to get value of variable " + name +
                                " as " + boost::typeindex::type_id<T>().pretty_name() + ": no logic for converting variable type " + type);
            }
            return result;
        }
    }
}

#endif //NGEN_BMI_UTILITIES_HPP
