#include <exception>
#include <utility>

#include "FileChecker.h"
#include "Bmi_C_Adapter.hpp"
#include "boost/algorithm/string.hpp"

using namespace models::bmi;

Bmi_C_Adapter::Bmi_C_Adapter(const string &type_name, std::string library_file_path, std::string forcing_file_path,
                             bool allow_exceed_end, bool has_fixed_time_step,
                             const std::string& registration_func, utils::StreamHandler output)
        : Bmi_C_Adapter(type_name, std::move(library_file_path), "", std::move(forcing_file_path),
                        allow_exceed_end, has_fixed_time_step, registration_func, output) {}

Bmi_C_Adapter::Bmi_C_Adapter(const string &type_name, std::string library_file_path, std::string bmi_init_config,
                             std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                             std::string registration_func, utils::StreamHandler output)
        : Bmi_Adapter<C_Bmi>(type_name, bmi_init_config, forcing_file_path, allow_exceed_end, has_fixed_time_step, output),
          bmi_lib_file(std::move(library_file_path)),
          bmi_registration_function(std::move(registration_func))
{
    try {
        construct_and_init_backing_model_for_type();
        // Make sure this is set to 'true' after this function call finishes
        model_initialized = true;
        acquire_time_conversion_factor(bmi_model_time_convert_factor);
    }
    // Record the exception message before re-throwing to handle subsequent function calls properly
    catch (exception& e) {
        // Make sure this is set to 'true' after this function call finishes
        model_initialized = true;
        throw e;
    }
}

// TODO: since the dynamically loaded lib and model struct can't easily be copied (without risking breaking once
//  original object closes the handle for its dynamically loaded lib) it make more sense to remove the copy constructor.
// TODO: However, it may make sense to bring it back once it is possible to serialize and deserialize the model.
/*
Bmi_C_Adapter::Bmi_C_Adapter(Bmi_C_Adapter &adapter) : model_name(adapter.model_name),
                                                       allow_model_exceed_end_time(adapter.allow_model_exceed_end_time),
                                                       bmi_init_config(adapter.bmi_init_config),
                                                       bmi_lib_file(adapter.bmi_lib_file),
                                                       bmi_model(adapter.bmi_model),
                                                       bmi_model_has_fixed_time_step(
                                                               adapter.bmi_model_has_fixed_time_step),
                                                       bmi_model_time_convert_factor(
                                                               adapter.bmi_model_time_convert_factor),
                                                       bmi_model_uses_forcing_file(adapter.bmi_model_uses_forcing_file),
                                                       bmi_registration_function(adapter.bmi_registration_function),
                                                       forcing_file_path(adapter.forcing_file_path),
                                                       init_exception_msg(adapter.init_exception_msg),
                                                       input_var_names(adapter.input_var_names),
                                                       model_initialized(adapter.model_initialized),
                                                       output_var_names(adapter.output_var_names),
                                                       output(std::move(adapter.output))
{
    // TODO: simple copying of the open dynamic library handle may lead to unexpected behavior, so perhaps open new?

    // TODO: for that matter, copying the model struct as was done before probably is not valid and should really
         involve serialization/deserialization.
}
*/

Bmi_C_Adapter::Bmi_C_Adapter(Bmi_C_Adapter &&adapter) noexcept:
        Bmi_Adapter<C_Bmi>(std::move(adapter)),
        bmi_lib_file(std::move(adapter.bmi_lib_file)),
        bmi_registration_function(adapter.bmi_registration_function),
        dyn_lib_handle(adapter.dyn_lib_handle)
{
    // Have to make sure to do this after "moving" so the original does not close the dynamically loaded library handle
    adapter.dyn_lib_handle = nullptr;
}

/**
 * Class destructor.
 *
 * Note that this calls the `Finalize()` function for cleaning up this object and its backing BMI model.
 */
Bmi_C_Adapter::~Bmi_C_Adapter() {
    finalizeForSubtype();
}

/**
 * Perform tear-down task for this object and its backing model.
 *
 * The function will simply return if either the pointer to the backing model is `nullptr` (e.g., after use
 * in a move constructor) or if the model has not been initialized.  Otherwise, it will execute its internal
 * tear-down logic, including a nested call to `finalize()` for the backing model.
 *
 * Note that because of how model initialization state is determined, regardless of whether the call to the
 * model's `finalize()` function is successful (i.e., according to the function's return code value), the
 * model will subsequently be consider not initialized.  This essentially means that if backing model
 * tear-down fails, it cannot be retried.
 *
 * @throws models::external::State_Exception Thrown if nested model `finalize()` call is not successful.
 */
void Bmi_C_Adapter::Finalize() {
    finalizeForSubtype();
}

/**
 * A non-virtual equivalent for the virtual @see Finalize.
 *
 * Primarily, this exists to contain the functionality appropriate for @see Finalize in a function that is
 * non-virtual, and can therefore be called by a destructor.
 */
void Bmi_C_Adapter::finalizeForSubtype() {
    if (bmi_model != nullptr && model_initialized) {
        model_initialized = false;
        int result = bmi_model->finalize(bmi_model.get());
        if (result != BMI_SUCCESS) {
            throw models::external::State_Exception("Failure attempting to finalize BMI C model " + model_name);
        }
    }
    // Then close the dynamically loaded library
    if (dyn_lib_handle != nullptr) {
        dlclose(dyn_lib_handle);
    }
}

std::shared_ptr<std::vector<std::string>> Bmi_C_Adapter::get_variable_names(bool is_input_variable) {
    int count;
    int count_result;
    if (is_input_variable)
        count_result = bmi_model->get_input_item_count(bmi_model.get(), &count);
    else
        count_result = bmi_model->get_output_item_count(bmi_model.get(), &count);
    if (count_result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to count of output variable names array.");
    }
    std::shared_ptr<std::vector<std::string>> var_names = std::make_shared<std::vector<std::string>>(std::vector<std::string>(count));

    char* names_array[count];
    for (int i = 0; i < count; i++) {
        names_array[i] = static_cast<char *>(malloc(sizeof(char) * BMI_MAX_VAR_NAME));
    }
    int names_result;
    if (is_input_variable) {
        names_result = bmi_model->get_input_var_names(bmi_model.get(), names_array);
    }
    else {
        names_result = bmi_model->get_output_var_names(bmi_model.get(), names_array);
    }
    if (names_result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get array of output variables names.");
    }

    for (int i = 0; i < count; ++i) {
        (*var_names)[i] = std::string(names_array[i]);
        free(names_array[i]);
    }
    return var_names;
}

string Bmi_C_Adapter::GetComponentName() {
    char component_name[BMI_MAX_COMPONENT_NAME];
    if (bmi_model->get_component_name(bmi_model.get(), component_name) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model component name.");
    }
    return {component_name};
}

double Bmi_C_Adapter::GetCurrentTime() {
    double current_time;
    int result = bmi_model->get_current_time(bmi_model.get(), &current_time);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get current model time.");
    }
    return current_time;
}

double Bmi_C_Adapter::GetEndTime() {
    double end_time;
    int result = bmi_model->get_end_time(bmi_model.get(), &end_time);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model end time.");
    }
    return end_time;
}

int Bmi_C_Adapter::GetInputItemCount() {
    return GetInputVarNames().size();
}

std::vector<std::string> Bmi_C_Adapter::GetInputVarNames() {
    if (input_var_names == nullptr) {
        input_var_names = get_variable_names(true);
    }

    return *input_var_names;
}

int Bmi_C_Adapter::GetOutputItemCount() {
    return GetOutputVarNames().size();
}

std::vector<std::string> Bmi_C_Adapter::GetOutputVarNames() {
    if (output_var_names == nullptr) {
        output_var_names = get_variable_names(false);
    }

    return *output_var_names;
}

double Bmi_C_Adapter::GetStartTime() {
    double start_time;
    int result = bmi_model->get_start_time(bmi_model.get(), &start_time);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model start time.");
    }
    return start_time;
}

std::string Bmi_C_Adapter::GetTimeUnits() {
    char time_units_cstr[BMI_MAX_UNITS_NAME];
    int result = bmi_model->get_time_units(bmi_model.get(), time_units_cstr);
    if (result != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to read time units from model.");
    }
    return std::string(time_units_cstr);
}

void Bmi_C_Adapter::GetValueAtIndices(std::string name, void *dest, int *inds, int count) {
    if (bmi_model->get_value_at_indices(bmi_model.get(), name.c_str(), dest, inds, count) != BMI_SUCCESS) {
        throw models::external::State_Exception(model_name + " failed to get variable " + name + " from model.");
    }
}

int Bmi_C_Adapter::GetVarItemsize(std::string name) {
    int size;
    int success = bmi_model->get_var_itemsize(bmi_model.get(), name.c_str(), &size);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable item size for " + name + ".");
    }
    return size;
}

int Bmi_C_Adapter::GetVarNbytes(std::string name) {
    int size;
    int success = bmi_model->get_var_nbytes(bmi_model.get(), name.c_str(), &size);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable array size (i.e., nbytes) for " + name + ".");
    }
    return size;
}

std::string Bmi_C_Adapter::GetVarType(std::string name) {
    char type_c_str[BMI_MAX_TYPE_NAME];
    int success = bmi_model->get_var_type(bmi_model.get(), name.c_str(), type_c_str);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable type for " + name + ".");
    }
    return std::string(type_c_str);
}

std::string Bmi_C_Adapter::GetVarUnits(std::string name) {
    char units_c_str[BMI_MAX_UNITS_NAME];
    int success = bmi_model->get_var_units(bmi_model.get(), name.c_str(), units_c_str);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable units for " + name + ".");
    }
    return std::string(units_c_str);
}

std::string Bmi_C_Adapter::GetVarLocation(std::string name) {
    char location_c_str[BMI_MAX_LOCATION_NAME];
    int success = bmi_model->get_var_location(bmi_model.get(), name.c_str(), location_c_str);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable location for " + name + ".");
    }
    return std::string(location_c_str);
}

int Bmi_C_Adapter::GetVarGrid(std::string name) {
    int grid;
    int success = bmi_model->get_var_grid(bmi_model.get(), name.c_str(), &grid);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable grid for " + name + ".");
    }
    return grid;
}

std::string Bmi_C_Adapter::GetGridType(int grid_id) {
    char gridtype_c_str[BMI_MAX_TYPE_NAME];
    int success = bmi_model->get_grid_type(bmi_model.get(), grid_id, gridtype_c_str);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid type for grid ID " + to_string(grid_id) + ".");
    }
    return std::string(gridtype_c_str);
}

int Bmi_C_Adapter::GetGridRank(int grid_id) {
    int gridrank;
    int success = bmi_model->get_grid_rank(bmi_model.get(), grid_id, &gridrank);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid rank for grid ID " + to_string(grid_id) + ".");
    }
    return gridrank;
}

int Bmi_C_Adapter::GetGridSize(int grid_id) {
    int gridsize;
    int success = bmi_model->get_grid_size(bmi_model.get(), grid_id, &gridsize);
    if (success != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid size for grid ID " + to_string(grid_id) + ".");
    }
    return gridsize;
}

void Bmi_C_Adapter::SetValue(std::string name, void *src) {
    int result = bmi_model->set_value(bmi_model.get(), name.c_str(), src);
    if (result != BMI_SUCCESS) {
        throw models::external::State_Exception("Failed to set values of " + name + " variable for " + model_name);
    }
}

bool Bmi_C_Adapter::is_model_initialized() {
    return model_initialized;
}

void Bmi_C_Adapter::SetValueAtIndices(std::string name, int *inds, int count, void *src) {
    int result = bmi_model->set_value_at_indices(bmi_model.get(), name.c_str(), inds, count, src);
    if (result != BMI_SUCCESS) {
        throw models::external::State_Exception(
                "Failed to set specified indexes for " + name + " variable of " + model_name);
    }
}

/**
 * Have the backing model update to next time step.
 *
 * Have the backing BMI model perform an update to the next time step according to its own internal time keeping.
 */
void Bmi_C_Adapter::Update() {
    int result = bmi_model->update(bmi_model.get());
    if (result != BMI_SUCCESS) {
        throw models::external::State_Exception("BMI C model execution update failed for " + model_name);
    }
}

/**
 * Have the backing BMI model update to the specified time.
 *
 * Update the backing BMI model to some desired model time, specified either explicitly or implicitly as a
 * non-integral multiple of time steps.  Note that the former is supported, but not required, by the BMI
 * specification.  The same is true for negative argument values.
 *
 * This function does not attempt to determine whether the particular backing model will consider the
 * provided parameter valid.
 *
 * @param time Time to update model to, either as literal model time or non-integral multiple of time steps.
 */
void Bmi_C_Adapter::UpdateUntil(double time) {
    int result = bmi_model->update_until(bmi_model.get(), time);
    if (result != BMI_SUCCESS) {
        throw models::external::State_Exception("Model execution update to specified time failed for " + model_name);
    }
}

void Bmi_C_Adapter::GetGridShape(const int grid, int *shape) {
    if (bmi_model->get_grid_shape(bmi_model.get(), grid, shape) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " shape.");
    }
}

void Bmi_C_Adapter::GetGridSpacing(const int grid, double *spacing) {
    if (bmi_model->get_grid_spacing(bmi_model.get(), grid, spacing) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " spacing.");
    }
}

void Bmi_C_Adapter::GetGridOrigin(const int grid, double *origin) {
    if (bmi_model->get_grid_origin(bmi_model.get(), grid, origin) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " origin.");
    }
}

void Bmi_C_Adapter::GetGridX(const int grid, double *x) {
    if (bmi_model->get_grid_x(bmi_model.get(), grid, x) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " x.");
    }
}

void Bmi_C_Adapter::GetGridY(const int grid, double *y) {
    if (bmi_model->get_grid_y(bmi_model.get(), grid, y) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " y.");
    }
}

void Bmi_C_Adapter::GetGridZ(const int grid, double *z) {
    if (bmi_model->get_grid_z(bmi_model.get(), grid, z) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " z.");
    }
}

int Bmi_C_Adapter::GetGridNodeCount(const int grid) {
    int count;
    if (bmi_model->get_grid_node_count(bmi_model.get(), grid, &count) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " node count.");
    }
    return count;
}

int Bmi_C_Adapter::GetGridEdgeCount(const int grid) {
    int count;
    if (bmi_model->get_grid_edge_count(bmi_model.get(), grid, &count) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " edge count.");
    }
    return count;
}

int Bmi_C_Adapter::GetGridFaceCount(const int grid) {
    int count;
    if (bmi_model->get_grid_face_count(bmi_model.get(), grid, &count) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " face count.");
    }
    return count;
}

void Bmi_C_Adapter::GetGridEdgeNodes(const int grid, int *edge_nodes) {
    if (bmi_model->get_grid_edge_nodes(bmi_model.get(), grid, edge_nodes) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " edge nodes.");
    }
}

void Bmi_C_Adapter::GetGridFaceEdges(const int grid, int *face_edges) {
    if (bmi_model->get_grid_face_edges(bmi_model.get(), grid, face_edges) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " face edges.");
    }
}

void Bmi_C_Adapter::GetGridFaceNodes(const int grid, int *face_nodes) {
    if (bmi_model->get_grid_face_nodes(bmi_model.get(), grid, face_nodes) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " face nodes.");
    }
}

void Bmi_C_Adapter::GetGridNodesPerFace(const int grid, int *nodes_per_face) {
    if (bmi_model->get_grid_nodes_per_face(bmi_model.get(), grid, nodes_per_face) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " nodes per face.");
    }
}
