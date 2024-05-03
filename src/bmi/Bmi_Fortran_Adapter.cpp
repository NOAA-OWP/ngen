#include <NGenConfig.h>

#if NGEN_WITH_BMI_FORTRAN
#include "bmi/Bmi_Fortran_Adapter.hpp"

using namespace models::bmi;

std::string Bmi_Fortran_Adapter::GetComponentName() {
    char component_name[BMI_MAX_COMPONENT_NAME];
    if (get_component_name(&bmi_model->handle, component_name) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model component name.");
    }
    return {component_name};
}

double Bmi_Fortran_Adapter::GetCurrentTime() {
    double current_time;
    if (get_current_time(&bmi_model->handle, &current_time) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get current model time.");
    }
    return current_time;
}

double Bmi_Fortran_Adapter::GetEndTime() {
    double end_time;
    if (get_end_time(&bmi_model->handle, &end_time) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model end time.");
    }
    return end_time;
}

std::vector<std::string> Bmi_Fortran_Adapter::GetInputVarNames() {
    if (input_var_names == nullptr) {
        input_var_names = inner_get_variable_names(true);
    }
    return *input_var_names;
}

std::vector<std::string> Bmi_Fortran_Adapter::GetOutputVarNames() {
    if (output_var_names == nullptr) {
        output_var_names = inner_get_variable_names(false);
    }
    return *output_var_names;
}

double Bmi_Fortran_Adapter::GetStartTime() {
    double start_time;
    if (get_start_time(&bmi_model->handle, &start_time) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model start time.");
    }
    return start_time;
}

double Bmi_Fortran_Adapter::GetTimeStep() {
    // TODO: go back and revisit this
    //return *get_bmi_model_time_step_size_ptr();
    double ts;
    if (get_time_step(&bmi_model->handle, &ts) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get model time step size.");
    }
    return ts;
}

std::string Bmi_Fortran_Adapter::GetTimeUnits() {
    char time_units_cstr[BMI_MAX_UNITS_NAME];
    if (get_time_units(&bmi_model->handle, time_units_cstr) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to read time units from model.");
    }
    return {time_units_cstr};
}

void Bmi_Fortran_Adapter::GetValueAtIndices(std::string name, void *dest, int *inds, int count) {
    // TODO: implement later by manually handling index convertion on this level.
    /*
    if (get_value_at_indices(&bmi_model->handle, name.c_str(), dest, inds, count) != BMI_SUCCESS) {
        throw models::external::State_Exception(
                model_name + " failed to get variable " + name + " for " + std::to_string(count) +
                " indices.");
    }
     */
    throw std::runtime_error("Fortran BMI module integration does not currently support getting values by index");
}

int Bmi_Fortran_Adapter::GetVarItemsize(std::string name) {
    int size;
    if (get_var_itemsize(&bmi_model->handle, name.c_str(), &size) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable item size for " + name + ".");
    }
    return size;
}

int Bmi_Fortran_Adapter::GetVarNbytes(std::string name) {
    int size;
    if (get_var_nbytes(&bmi_model->handle, name.c_str(), &size) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable array size (i.e., nbytes) for " + name + ".");
    }
    return size;
}

std::string Bmi_Fortran_Adapter::GetVarType(std::string name) {
    return inner_get_var_type(name);
}

std::string Bmi_Fortran_Adapter::GetVarUnits(std::string name) {
    char units_c_str[BMI_MAX_UNITS_NAME];
    if (get_var_units(&bmi_model->handle, name.c_str(), units_c_str) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable units for " + name + ".");
    }
    return std::string(units_c_str);
}

std::string Bmi_Fortran_Adapter::GetVarLocation(std::string name) {
    char location_c_str[BMI_MAX_LOCATION_NAME];
    if (get_var_location(&bmi_model->handle, name.c_str(), location_c_str) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable location for " + name + ".");
    }
    return std::string(location_c_str);
}

int Bmi_Fortran_Adapter::GetVarGrid(std::string name) {
    int grid;
    if (get_var_grid(&bmi_model->handle, name.c_str(), &grid) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get variable grid for " + name + ".");
    }
    return grid;
}

std::string Bmi_Fortran_Adapter::GetGridType(int grid_id) {
    char gridtype_c_str[BMI_MAX_TYPE_NAME];
    if (get_grid_type(&bmi_model->handle, &grid_id, gridtype_c_str) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid type for grid ID " + std::to_string(grid_id) + ".");
    }
    return std::string(gridtype_c_str);
}

int Bmi_Fortran_Adapter::GetGridRank(int grid_id) {
    int gridrank;
    if (get_grid_rank(&bmi_model->handle, &grid_id, &gridrank) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid rank for grid ID " + std::to_string(grid_id) + ".");
    }
    return gridrank;
}

int Bmi_Fortran_Adapter::GetGridSize(int grid_id) {
    int gridsize;
    if (get_grid_size(&bmi_model->handle, &grid_id, &gridsize) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid size for grid ID " + std::to_string(grid_id) + ".");
    }
    return gridsize;
}

void Bmi_Fortran_Adapter::SetValue(std::string name, void *src) {
    inner_set_value(name, src);
}

bool Bmi_Fortran_Adapter::is_model_initialized() {
    return model_initialized;
}

void Bmi_Fortran_Adapter::SetValueAtIndices(std::string name, int *inds, int count, void *src) {
    // TODO: implement later by manually handling index convertion on this level.
    /*
    int result = set_value_at_indices(&bmi_model->handle, name.c_str(), inds, count, src);
    if (result != BMI_SUCCESS) {
        throw models::external::State_Exception(
                "Failed to set specified indexes for " + name + " variable of " + model_name);
    }
    */
    throw std::runtime_error("Fortran BMI module integration does not currently support setting values by index");
}

/**
 * Have the backing model update to next time step.
 *
 * Have the backing BMI model perform an update to the next time step according to its own internal time keeping.
 */
void Bmi_Fortran_Adapter::Update() {
    if (update(&bmi_model->handle) != BMI_SUCCESS) {
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
void Bmi_Fortran_Adapter::UpdateUntil(double time) {
    if (update_until(&bmi_model->handle, &time) != BMI_SUCCESS) {
        throw models::external::State_Exception("Model execution update to specified time failed for " + model_name);
    }
}

void Bmi_Fortran_Adapter::GetGridShape(int grid, int *shape) {
    if (get_grid_shape(&bmi_model->handle, &grid, shape) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " shape.");
    }
}

void Bmi_Fortran_Adapter::GetGridSpacing(int grid, double *spacing) {
    if (get_grid_spacing(&bmi_model->handle, &grid, spacing) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " spacing.");
    }
}

void Bmi_Fortran_Adapter::GetGridOrigin(int grid, double *origin) {
    if (get_grid_origin(&bmi_model->handle, &grid, origin) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " origin.");
    }
}

void Bmi_Fortran_Adapter::GetGridX(int grid, double *x) {
    if (get_grid_x(&bmi_model->handle, &grid, x) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " x.");
    }
}

void Bmi_Fortran_Adapter::GetGridY(int grid, double *y) {
    if (get_grid_y(&bmi_model->handle, &grid, y) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " y.");
    }
}

void Bmi_Fortran_Adapter::GetGridZ(int grid, double *z) {
    if (get_grid_z(&bmi_model->handle, &grid, z) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " z.");
    }
}

int Bmi_Fortran_Adapter::GetGridNodeCount(int grid) {
    int count;
    if (get_grid_node_count(&bmi_model->handle, &grid, &count) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " node count.");
    }
    return count;
}

int Bmi_Fortran_Adapter::GetGridEdgeCount(int grid) {
    int count;
    if (get_grid_edge_count(&bmi_model->handle, &grid, &count) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " edge count.");
    }
    return count;
}

int Bmi_Fortran_Adapter::GetGridFaceCount(int grid) {
    int count;
    if (get_grid_face_count(&bmi_model->handle, &grid, &count) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " face count.");
    }
    return count;
}

void Bmi_Fortran_Adapter::GetGridEdgeNodes(int grid, int *edge_nodes) {
    if (get_grid_edge_nodes(&bmi_model->handle, &grid, edge_nodes) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " edge nodes.");
    }
}

void Bmi_Fortran_Adapter::GetGridFaceEdges(int grid, int *face_edges) {
    if (get_grid_face_edges(&bmi_model->handle, &grid, face_edges) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " face edges.");
    }
}

void Bmi_Fortran_Adapter::GetGridFaceNodes(int grid, int *face_nodes) {
    if (get_grid_face_nodes(&bmi_model->handle, &grid, face_nodes) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " face nodes.");
    }
}

void Bmi_Fortran_Adapter::GetGridNodesPerFace(int grid, int *nodes_per_face) {
    if (get_grid_nodes_per_face(&bmi_model->handle, &grid, nodes_per_face) != BMI_SUCCESS) {
        throw std::runtime_error(model_name + " failed to get grid " + std::to_string(grid) + " nodes per face.");
    }
}

#endif // NGEN_WITH_BMI_FORTRAN
