#include <NetcdfOutputWriterUtils.hpp>
#include <Layer.hpp>
#include <DomainLayer.hpp>
#include <utilities/logging_utils.h>
#include <unordered_set>

void add_dimensions_for_layer(dimension_discription_vec& dim_desc, std::shared_ptr<ngen::Layer> layer, unsigned long num_catchments, unsigned long num_nexuses )
{
    dim_desc.push_back(data_output::NetcdfDimensionDiscription("time"));

    int layer_class_id = layer->class_id();
    switch(layer_class_id)
    {
        // setup variables and description for surface layers
        case ngen::LayerClass::kSurfaceLayer:
        {
            dim_desc.push_back(data_output::NetcdfDimensionDiscription("catchment-id",num_catchments));
        }
        break;

        // setup variables and description for domain layers
        case ngen::LayerClass::kDomainLayer:
        {
            std::shared_ptr<ngen::DomainLayer> domain_layer = std::dynamic_pointer_cast<ngen::DomainLayer,ngen::Layer>(layer);

            dim_desc.push_back(data_output::NetcdfDimensionDiscription("x"));
            dim_desc.push_back(data_output::NetcdfDimensionDiscription("y"));   
        }
        break;

        // setup variables and description for catchment layers
        case ngen::LayerClass::kCatchmentLayer:
        {
            dim_desc.push_back(data_output::NetcdfDimensionDiscription("catchment-id",num_catchments));
        }
        break;

        // setup variables and description for nexus layers
        case ngen::LayerClass::kNexusLayer:
        {
            dim_desc.push_back(data_output::NetcdfDimensionDiscription("nexus-id",num_nexuses));
        }
        break;

        // setup variables and description for surface layers
        case ngen::LayerClass::kLayer:
        {
            // basicly layer should probably never be reached so log warning
            logging::warning("Layer of class gen::LayerClass::kLayer encountered this is a base class not intended for direct use\n");

            dim_desc.push_back(data_output::NetcdfDimensionDiscription("catchment-id",num_catchments));
        }
        break;

        default:
        {
            std::stringstream ss;
            ss << "Unexepected Layer class encountered with id " << layer_class_id << "\n";
            throw std::runtime_error(ss.str().c_str());
        }
        break;
    }    
}


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