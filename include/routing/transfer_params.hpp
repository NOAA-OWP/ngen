#ifndef NGEN_ROUTING_TRANSFER_H
#define NGEN_ROUTING_TRANSFER_H

#if NGEN_WITH_ROUTING

#include "Bmi_Py_Adapter.hpp"

namespace models {
    namespace bmi {
        void transfer_py_bmi_data(std::string var_name, Bmi_Py_Adapter &source, Bmi_Py_Adapter &dest);
        void transfer_py_bmi_data(std::string source_var_name, Bmi_Py_Adapter &source, std::string dest_var_name, Bmi_Py_Adapter &dest);
    }
}

#endif // NGEN_WITH_ROUTING

#endif
