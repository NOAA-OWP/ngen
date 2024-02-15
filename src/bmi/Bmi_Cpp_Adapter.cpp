#include <exception>
#include <utility>

#include "FileChecker.h"
#include "Bmi_Cpp_Adapter.hpp"

using namespace models::bmi;

Bmi_Cpp_Adapter::Bmi_Cpp_Adapter(const std::string& type_name, std::string library_file_path, std::string forcing_file_path,
                             bool allow_exceed_end, bool has_fixed_time_step,
                             std::string creator_func, std::string destroyer_func,
                             utils::StreamHandler output)
        : Bmi_Cpp_Adapter(type_name, std::move(library_file_path), "", std::move(forcing_file_path),
                        allow_exceed_end, has_fixed_time_step, creator_func, destroyer_func, output) { }

Bmi_Cpp_Adapter::Bmi_Cpp_Adapter(const std::string& type_name, std::string library_file_path, std::string bmi_init_config,
                             std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                             std::string creator_func, std::string destroyer_func,
                             utils::StreamHandler output)
        : Bmi_Cpp_Adapter(type_name, std::move(library_file_path), std::move(bmi_init_config),
                        std::move(forcing_file_path), allow_exceed_end, has_fixed_time_step,
                        std::move(creator_func), std::move(destroyer_func), output, true) { }

Bmi_Cpp_Adapter::Bmi_Cpp_Adapter(const std::string& type_name, std::string library_file_path, std::string bmi_init_config,
                             std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                             std::string creator_func, std::string destroyer_func,
                             utils::StreamHandler output, bool do_initialization)
                             : AbstractCLibBmiAdapter(type_name, library_file_path, std::move(bmi_init_config), std::move(forcing_file_path), allow_exceed_end,
                             has_fixed_time_step, creator_func, output),
                             model_create_fname(std::move(creator_func)),
                             model_destroy_fname(std::move(destroyer_func))
                             //TODO: We are passing creator_func as registration_func because AbstractCLibBmiAdapter expects it to exist, but are not using it the same way...may be okay but we may want to generalize that assumption out!
{
    if (do_initialization) {
        try {
            construct_and_init_backing_model_for_type();
            // Make sure this is set to 'true' after this function call finishes
            model_initialized = true;
            bmi_model_time_convert_factor = get_time_convert_factor();
        }
        // Record the exception message before re-throwing to handle subsequent function calls properly
        catch (const std::exception &e) {
            std::clog << e.what() << std::endl;
            model_initialized = true;
            throw e;
        }
        catch (...) {
            const std::exception_ptr& e = std::current_exception();
            // Make sure this is set to 'true' after this function call finishes
            //TODO: This construct may not be necessary for the C++ adapter because the shared_ptr has a lambda set up to destroy the model?
            model_initialized = true;
            throw e;
        }
    }
}

// TODO: since the dynamically loaded lib and model struct can't easily be copied (without risking breaking once
//  original object closes the handle for its dynamically loaded lib) it make more sense to remove the copy constructor.
// TODO: However, it may make sense to bring it back once it is possible to serialize and deserialize the model.
/*
Bmi_Cpp_Adapter::Bmi_Cpp_Adapter(Bmi_Cpp_Adapter &adapter) :
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

std::string Bmi_Cpp_Adapter::GetComponentName() {
    return bmi_model->GetComponentName();
}

double Bmi_Cpp_Adapter::GetCurrentTime() {
    return bmi_model->GetCurrentTime();
}

double Bmi_Cpp_Adapter::GetEndTime() {
    return bmi_model->GetEndTime();
}

int Bmi_Cpp_Adapter::GetInputItemCount() {
    return bmi_model->GetInputItemCount();
}

std::vector<std::string> Bmi_Cpp_Adapter::GetInputVarNames() {
    return bmi_model->GetInputVarNames();
}

int Bmi_Cpp_Adapter::GetOutputItemCount() {
    return bmi_model->GetOutputItemCount();
}

std::vector<std::string> Bmi_Cpp_Adapter::GetOutputVarNames() {
    return bmi_model->GetOutputVarNames();
}

double Bmi_Cpp_Adapter::GetStartTime() {
    return bmi_model->GetStartTime();
}

std::string Bmi_Cpp_Adapter::GetTimeUnits() {
    return bmi_model->GetTimeUnits();
}

void Bmi_Cpp_Adapter::GetValueAtIndices(std::string name, void *dest, int *inds, int count) {
    return bmi_model->GetValueAtIndices(name, dest, inds, count);
}

int Bmi_Cpp_Adapter::GetVarItemsize(std::string name) {
    return bmi_model->GetVarItemsize(name);
}

int Bmi_Cpp_Adapter::GetVarNbytes(std::string name) {
    return bmi_model->GetVarNbytes(name);
}

std::string Bmi_Cpp_Adapter::GetVarType(std::string name) {
    return bmi_model->GetVarType(name);
}

std::string Bmi_Cpp_Adapter::GetVarUnits(std::string name) {
    return bmi_model->GetVarUnits(name);
}

std::string Bmi_Cpp_Adapter::GetVarLocation(std::string name) {
    return bmi_model->GetVarLocation(name);
}

int Bmi_Cpp_Adapter::GetVarGrid(std::string name) {
    return bmi_model->GetVarGrid(name);
}

std::string Bmi_Cpp_Adapter::GetGridType(int grid_id) {
    return bmi_model->GetGridType(grid_id);
}

int Bmi_Cpp_Adapter::GetGridRank(int grid_id) {
    return bmi_model->GetGridRank(grid_id);
}

int Bmi_Cpp_Adapter::GetGridSize(int grid_id) {
    return bmi_model->GetGridSize(grid_id);
}

void Bmi_Cpp_Adapter::SetValue(std::string name, void *src) {
    bmi_model->SetValue(name, src);
}

bool Bmi_Cpp_Adapter::is_model_initialized() {
    return model_initialized;
}

void Bmi_Cpp_Adapter::SetValueAtIndices(std::string name, int *inds, int count, void *src) {
    bmi_model->SetValueAtIndices(name, inds, count, src);
}

void Bmi_Cpp_Adapter::Update() {
    bmi_model->Update();
}

void Bmi_Cpp_Adapter::UpdateUntil(double time) {
    bmi_model->UpdateUntil(time);
}

void Bmi_Cpp_Adapter::GetGridShape(const int grid, int *shape) {
    bmi_model->GetGridShape(grid, shape);
}

void Bmi_Cpp_Adapter::GetGridSpacing(const int grid, double *spacing) {
    bmi_model->GetGridSpacing(grid, spacing);
}

void Bmi_Cpp_Adapter::GetGridOrigin(const int grid, double *origin) {
    bmi_model->GetGridOrigin(grid, origin);
}

void Bmi_Cpp_Adapter::GetGridX(const int grid, double *x) {
    bmi_model->GetGridX(grid, x);
}

void Bmi_Cpp_Adapter::GetGridY(const int grid, double *y) {
    bmi_model->GetGridY(grid, y);
}

void Bmi_Cpp_Adapter::GetGridZ(const int grid, double *z) {
    bmi_model->GetGridZ(grid, z);
}

int Bmi_Cpp_Adapter::GetGridNodeCount(const int grid) {
    return bmi_model->GetGridNodeCount(grid);
}

int Bmi_Cpp_Adapter::GetGridEdgeCount(const int grid) {
    return bmi_model->GetGridEdgeCount(grid);
}

int Bmi_Cpp_Adapter::GetGridFaceCount(const int grid) {
    return bmi_model->GetGridFaceCount(grid);
}

void Bmi_Cpp_Adapter::GetGridEdgeNodes(const int grid, int *edge_nodes) {
    bmi_model->GetGridEdgeNodes(grid, edge_nodes);
}

void Bmi_Cpp_Adapter::GetGridFaceEdges(const int grid, int *face_edges) {
    bmi_model->GetGridFaceEdges(grid, face_edges);
}

void Bmi_Cpp_Adapter::GetGridFaceNodes(const int grid, int *face_nodes) {
    bmi_model->GetGridFaceNodes(grid, face_nodes);
}

void Bmi_Cpp_Adapter::GetGridNodesPerFace(const int grid, int *nodes_per_face) {
    bmi_model->GetGridNodesPerFace(grid, nodes_per_face);
}
