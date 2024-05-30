#include <NetcdfOutputWriterUtils.hpp>

#include <unordered_set>

void add_variables_for_layer(variable_discription_vec& var_vec, std::shared_ptr<ngen::Layer> layer)
{
    std::unordered_set<std::string> variable_names;
    
    int layer_class_id = layer->class_id();
    
    // for each contained member of the layer get the stored objects
    for( auto & id : layer->get_contents() )
    {
        // retrieve a pointer to realization 
        auto fm = layer->get_realization(id);

        // cast the pointer to a bmi type
        auto bmi_f = std::dynamic_pointer_cast<realization::Bmi_Formulation>(fm);

        std::vector<std::string> catchment_list;
        catchment_list.push_back("time");
        catchment_list.push_back("catchment-id");

        std::vector<std::string> grid_list;
        grid_list.push_back("time");
        grid_list.push_back("x");
        grid_list.push_back("y");

        std::vector<std::string> nexus_list;
        nexus_list.push_back("time");
        nexus_list.push_back("nexus-id");

        try
        {
            // get the variable names associated with this realization
            auto names = bmi_f->get_output_variable_names();

            for( auto name : names )
            {
                // check to see if the name is assigned
                if ( variable_names.find(name) != variable_names.end() )
                {
                    variable_names.insert(name);

                    data_output::NetcdfVariableDiscription var_desc;

                    var_desc.name = name;
                    
                    var_desc.type = bmi_f->get_output_type(name);

                    switch(layer_class_id)
                    {
                        case ngen::LayerClass::kSurfaceLayer:

                        var_desc.dim_names = catchment_list;

                        break;

                        case ngen::LayerClass::kDomainLayer:

                        var_desc.dim_names = grid_list;

                        break;

                        case ngen::LayerClass::kCatchmentLayer:

                        var_desc.dim_names = catchment_list;

                        break;

                        case ngen::LayerClass::kNexusLayer:

                        var_desc.dim_names = nexus_list;

                        break;

                        case ngen::LayerClass::kLayer:

                        var_desc.dim_names = catchment_list;

                        break;

                        default:

                        throw std::runtime_error("Unexpected Layer Class");

                        break;
                    }

                    var_vec.push_back(var_desc);

                }
            }
        }
        catch (std::exception e)
        {
            throw(e);
        }
    }
}