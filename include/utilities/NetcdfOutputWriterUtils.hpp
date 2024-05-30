#ifndef __NETCDF_OUTPUT_WRITER_UTILS__
#define __NETCDF_OUTPUT_WRITER_UTILS__

#include <core/Layer.hpp>
#include <output/NetcdfOutputWriter.hpp>

using dimension_discription_vec = std::vector<data_output::NetcdfDimensionDiscription>;
using variable_discription_vec = std::vector<data_output::NetcdfVariableDiscription>;
void add_variables_for_layer(variable_discription_vec& var_desc, std::shared_ptr<ngen::Layer> layer);


#endif