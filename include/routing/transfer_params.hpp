#ifndef NGEN_ROUTING_TRANSFER_H
#define NGEN_ROUTING_TRANSFER_H

#include "Bmi_Py_Adapter.hpp"

namespace models {
    namespace bmi {
        /** 
         * @brief Transfer the BMI data from one T-route BMI to another.
         * Bmi_Py_Adapter does not allow for changing the size of the data through SetValue. Adding "__count" to a var name in troute and daforcing BMIs will signal the BMI to resize its current data to allow a new data size to be set.
         * @param var_name Variable name of the data that will be transfered.
         * @param source Source BMI.
         * @param dest Destination BMI.
        */
        void transfer_py_bmi_data(std::string var_name, Bmi_Py_Adapter &source, Bmi_Py_Adapter &dest) {
            // get the number of items in the source
            int item_size = source.GetVarItemsize(var_name);
            int nbytes = source.GetVarNbytes(var_name);
            int count = nbytes / item_size;

            // resize destination data to be equivalent to the source
            std::string var_name_count = var_name + "__count";
            dest.SetValue(var_name_count, &count);

            // get source data and transfer it
            void *data = source.GetValuePtr(var_name);
            dest.SetValue(var_name, data);
        }

        /**
         * @brief Transfer the BMI data from one T-route BMI to another.
         * Bmi_Py_Adapter does not allow for changing the size of the data through SetValue. Adding "__count" to a var name in troute and daforcing BMIs will signal the BMI to resize its current data to allow a new data size to be set.
         * @param source_var_name Variable name of the data from the source BMI.
         * @param source Source BMI.
         * @param dest_var_name Variable name for the destination BMI.
         * @param dest Destination BMI.
         */
        void transfer_py_bmi_data(std::string source_var_name, Bmi_Py_Adapter &source, std::string dest_var_name, Bmi_Py_Adapter &dest) {
            // get the number of items in the source
            int item_size = source.GetVarItemsize(source_var_name);
            int nbytes = source.GetVarNbytes(source_var_name);
            int count = nbytes / item_size;

            // resize destination data to be equivalent to the source
            std::string var_name_count = dest_var_name + "__count";
            dest.SetValue(var_name_count, &count);

            // get source data and transfer it
            void *data = source.GetValuePtr(source_var_name);
            dest.SetValue(dest_var_name, data);
        }
    }
}

#endif
