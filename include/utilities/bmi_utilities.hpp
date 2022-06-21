#ifndef NGEN_BMI_UTILITIES_HPP
#define NGEN_BMI_UTILITIES_HPP

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
         * @brief Copy values from a BMI model adapter and box them into a vector.
         * 
         * @tparam T Type to cast data from the BMI model to
         * @tparam A Bmi model type
         * @param model Bmi model adapter that interfaces to @tparam A bmi
         * @param name Bmi variable name to query the model for
         * @return std::vector<T> Copy of data from the BMI model for variable @param name
         */
        template <typename T, typename A>
        std::vector<T> GetValue(Bmi_Adapter<A>& model, const std::string& name) {
            //TODO make model const ref
            int total_mem = model.GetVarNbytes(name);
            int item_size = model.GetVarItemsize(name);
            int num_items = total_mem/item_size;
            //Determine what type we need to cast from
            std::string type = model.get_analogous_cxx_type(model.GetVarType(name), item_size);
            
            //C++ form of malloc
            void* data = ::operator new(total_mem);
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
