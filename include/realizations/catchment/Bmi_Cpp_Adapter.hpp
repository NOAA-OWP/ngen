#ifndef NGEN_BMI_CPP_ADAPTER_H
#define NGEN_BMI_CPP_ADAPTER_H

#include <memory>
#include <string>
#include "AbstractCLibBmiAdapter.hpp"
#include "bmi.hpp"
#include "JSONProperty.hpp"
#include "StreamHandler.hpp"

// Forward declaration to provide access to protected items in testing
class Bmi_Cpp_Adapter_Test;

using Cpp_Bmi = bmi::Bmi;

using ModelCreator = Cpp_Bmi* (*)();
using ModelDestroyer = void (*)(Cpp_Bmi*);

namespace models {
    namespace bmi {

        /**
         * A thin wrapper class to provide convenience functions around raw BMI models written in C++, especially those
         * loaded dynamically from libraries. This is less important than e.g. @see Bmi_C_Adapter but still provides
         * some useful generalized functionality.
         */
        class Bmi_Cpp_Adapter : public AbstractCLibBmiAdapter<Cpp_Bmi>  {

        public:

            /**
             * Public constructor without path to BMI initialization config file.
             *
             * @param type_name The name of the backing BMI module/model type.
             * @param library_file_path The string path to the shared library file for external module.
             * @param forcing_file_path The string path for the forcing file the module should use, empty if it does not
             *                          use one directly.
             * @param allow_exceed_end Whether the backing model is allowed to execute beyond its advertised end_time.
             * @param has_fixed_time_step Whether the model has a fixed time step size.
             * @param creator_func The name for the @see creator_function .
             * @param destoryer_func The name for the @see destroyer_function .
             * @param output The output stream handler.
             */
            explicit Bmi_Cpp_Adapter(const std::string &type_name, std::string library_file_path, std::string forcing_file_path,
                                   bool allow_exceed_end, bool has_fixed_time_step,
                                   std::string creator_func, std::string destroyer_func,
                                   utils::StreamHandler output);

            /**
             * Main public constructor.
             *
             * @param type_name The name of the backing BMI module/model type.
             * @param library_file_path The string path to the shared library file for external module.
             * @param bmi_init_config The string path to the BMI initialization config file for the module.
             * @param forcing_file_path The string path for the forcing file the module should use, empty if it does not
             *                          use one directly.
             * @param allow_exceed_end Whether the backing model is allowed to execute beyond its advertised end_time.
             * @param has_fixed_time_step Whether the model has a fixed time step size.
             * @param creator_func The name for the @see creator_function .
             * @param destoryer_func The name for the @see destroyer_function .
             * @param output The output stream handler.
             */
            Bmi_Cpp_Adapter(const std::string& type_name, std::string library_file_path, std::string bmi_init_config,
                          std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                          std::string creator_func, std::string destroyer_func,
                          utils::StreamHandler output);

        protected:

            /**
             * Protected constructor that allows control over whether initialization steps are done during construction.
             *
             * Constructor that has parameter to control whether initialization steps - i.e., steps that would be
             * performed in the BMI @see Initialize(std::string) function if not already done - are done when
             * constructing the object.
             *
             * In general, it is assumed that an object of this type will be initialized on construction.  However, a
             * subtype may wish to utilize the constructor while deferring initialization (usually so it can perform
             * that internally as a later step).
             *
             * @param type_name The name of the backing BMI module/model type.
             * @param library_file_path The string path to the shared library file for external module.
             * @param bmi_init_config The string path to the BMI initialization config file for the module.
             * @param forcing_file_path The string path for the forcing file the module should use, empty if it does not
             *                          use one directly.
             * @param allow_exceed_end Whether the backing model is allowed to execute beyond its advertised end_time.
             * @param has_fixed_time_step Whether the model has a fixed time step size.
             * @param creator_func The name for the @see creator_function .
             * @param destoryer_func The name for the @see destroyer_function .
             * @param output The output stream handler.
             * @param do_initialization Whether initialization should be performed during construction or deferred.
             */
            Bmi_Cpp_Adapter(const std::string& type_name, std::string library_file_path, std::string bmi_init_config,
                          std::string forcing_file_path, bool allow_exceed_end, bool has_fixed_time_step,
                          std::string creator_func, std::string destroyer_func,
                          utils::StreamHandler output, bool do_initialization);

        public:

            // Copy constructor
            // TODO: since the dynamically loaded lib and model struct can't easily be copied (without risking breaking
            //  once original object closes the handle for its dynamically loaded lib) it make more sense to remove the
            //  copy constructor.
            // TODO: However, it may make sense to bring it back once it's possible to serialize/deserialize the model.
            //Bmi_Cpp_Adapter(Bmi_Cpp_Adapter &adapter);

            // Move constructor
            Bmi_Cpp_Adapter(Bmi_Cpp_Adapter &&adapter) noexcept;

            /**
             * Class destructor.
             *
             * Note that this calls the `Finalize()` function for cleaning up this object and its backing BMI model.
             */
            virtual ~Bmi_Cpp_Adapter() {
                finalizeForCppAdapter();
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
            void Finalize() override {
                finalizeForCppAdapter();
            }

            string GetComponentName() override;

            /**
             * Get the backing model's current time.
             *
             * Get the backing model's current time, relative to the start time, in the units expressed by
             * `GetTimeUnits()`.
             *
             * @return The backing model's current time.
             */
            double GetCurrentTime() override;

            double GetEndTime() override;

            /**
             * The number of input variables the model can use.
             *
             * The number of input variables the model can use from other models implementing a BMI.
             *
             * @return The number of input variables the model can use from other models implementing a BMI.
             */
            int GetInputItemCount() override;

            /**
             * Get input variable names for the backing model.
             *
             * Gets a collection of names for the variables the model can use from other models implementing a BMI.
             *
             * @return A vector of names for the variables the model can use from other models implementing a BMI.
             */
            std::vector<std::string> GetInputVarNames() override;

            int GetOutputItemCount() override;

            std::vector<std::string> GetOutputVarNames() override;

            /**
             * Get the backing model's starting time.
             *
             * Get the backing model's starting time, in the units expressed by `GetTimeUnits()`.
             *
             * The BMI standard start time is typically defined to be `0.0`.
             *
             * @return
             */
            double GetStartTime() override;

            /**
             * Get the time step used in the model.
             *
             * Get the time step size used in the backing model, expressed as a `double` type value.
             *
             * This function defers to the behavior of the analogous function `get_time_step` of the backing model.  As
             * such, the the model-specific documentation for this function should be consulted for utilized models.
             *
             * @return The time step size of the model.
             */
            inline double GetTimeStep() override {
                return *get_bmi_model_time_step_size_ptr();
            }

            /**
             * Get the units of time for the backing model.
             *
             * Get the units of time for the backing model, as a standard convention string representing the unit
             * obtained directly from the model.
             *
             * @return A conventional string representing the time units for the model.
             */
            std::string GetTimeUnits() override;

            /**
             * Get value(s) for a variable.
             *
             * Implementation of virtual function in BMI interface.
             *
             * @param name
             * @param dest
             * @see get_value
             */
            void GetValue(std::string name, void *dest) override {
                return bmi_model->GetValue(name, dest);
            }

            void GetValueAtIndices(std::string name, void *dest, int *inds, int count) override;

            void GetGridShape(const int grid, int *shape) override;

            void GetGridSpacing(const int grid, double *spacing) override;

            void GetGridOrigin(const int grid, double *origin) override;

            void GetGridX(const int grid, double *x) override;

            void GetGridY(const int grid, double *y) override;

            void GetGridZ(const int grid, double *z) override;

            int GetGridNodeCount(const int grid) override;

            int GetGridEdgeCount(const int grid) override;

            int GetGridFaceCount(const int grid) override;

            void GetGridEdgeNodes(const int grid, int *edge_nodes) override;

            void GetGridFaceEdges(const int grid, int *face_edges) override;

            void GetGridFaceNodes(const int grid, int *face_nodes) override;

            void GetGridNodesPerFace(const int grid, int *nodes_per_face) override;

            void *GetValuePtr(std::string name) override {
                return bmi_model->GetValuePtr(name);
            }

            int GetVarItemsize(std::string name) override;

            int GetVarNbytes(std::string name) override;

            int GetVarGrid(std::string name) override;

            std::string GetVarType(std::string name) override;

            std::string GetVarUnits(std::string name) override;

            std::string GetVarLocation(std::string name) override;

            std::string GetGridType(int grid_id) override;

            int GetGridRank(int grid_id) override;

            int GetGridSize(int grid_id) override;

            template<class T>
            T *GetValuePtr(const std::string &name){
                return (T*) bmi_model->GetValuePtr(name);
            }

            /**
             * This is totally redundant for C++ models, so this simply returns the original
             * string for the external name.
             *
             * @param external_type_name The string name of a C++ type.
             * @param item_size The particular size in bytes for items of the involved analogous types.
             * @return The name string for the analogous C++ type, which is exactly what was passed in.
             */
            const std::string
            get_analogous_cxx_type(const std::string &external_type_name, const size_t item_size) override {
                return external_type_name;
            }

            /**
             * Whether the backing model has been initialized yet.
             *
             * @return Whether the backing model has been initialized yet.
             */
            bool is_model_initialized();

            void SetValue(std::string name, void *src) override;

            template<class T>
            void SetValue(std::string name, std::vector<T> src) {
                bmi_model->SetValue(name, src);
            }

            void SetValueAtIndices(std::string name, int *inds, int count, void *src) override;

            template<class T>
            void SetValueAtIndices(const std::string &name, std::vector<int> inds, std::vector<T> src) {
                bmi_model->SetValueAtIndices(name, inds, src);
            }

            void Update() override;

            void UpdateUntil(double time) override;

        protected:
            // TODO: look at setting this in some other way
            static const std::string model_name;

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * Implementations should return immediately without taking any further action if ``model_initialized`` is
             * already ``true``.
             *
             * The call to the BMI native ``Initialize(string)`` should pass the value stored in ``bmi_init_config``.
             * @see construct_and_init_backing_model_for_type
             */
            void construct_and_init_backing_model() override {
                construct_and_init_backing_model_for_type();
            }

            /**
             * Load and execute the creator function for the backing BMI module.
             *
             * Integrated BMI module libraries are expected to provide an additional ``bmi_model_create`` function. This
             * works like a constructor (or factory) for the model object and returns a pointer to the allocated model
             * instance.
             */
            inline void execModuleCreation() {
                if (get_dyn_lib_handle() == nullptr) {
                    if (model_create_fname.empty()) {
                        this->init_exception_msg =
                                "Can't init " + this->model_name + "; empty name given for module's create function.";
                        throw std::runtime_error(this->init_exception_msg);
                    }
                    if (model_destroy_fname.empty()){
                        this->init_exception_msg =
                                "Can't init " + this->model_name + "; empty name given for module's destroy function.";
                        throw std::runtime_error(this->init_exception_msg);
                    }
                    dynamic_library_load();
                }
                void *symbol;
                //Cpp_Bmi *(*dynamic_create_bmi)(Cpp_Bmi *model);
                ModelCreator dynamic_creator;

                try {
                    // Acquire the BMI creator function
                    symbol = dynamic_load_symbol(model_create_fname);
                    dynamic_creator = (ModelCreator) symbol;
                    bmi_model = std::shared_ptr<Cpp_Bmi>(
                        dynamic_creator(),
                        [](Cpp_Bmi* p) {
                            // DO NOTHING.
                            /* The best practice pattern for using C++ objects with dlopen is to declare a creator function
                             * AND a destroyer function. It is not wise to create the object in the library and destroy it
                             * in the calling environment because reasons (https://stackoverflow.com/a/40109212). Originally
                             * here I had a call to the destroyer function, but that blew up because in our implementation
                             * dlclose() gets called before this shared_ptr gets decremented to zero, so segfault happens
                             * when calling the library-resident destroyer function. It would probably be simpler to skip
                             * shared_ptr entirely and just keep a raw pointer to the object, but that makes it look like 
                             * we're being ignorant of smart pointers...so this is left here as an explicit NO-OP and the
                             * destroyer is called in Finalize() above, which occurs in the right sequence. Better ideas?
                             * 
                             * Note also that the creator function is called in our *constructor*, not in Initialize(), so
                             * this is somewhat asymmetric...however, BMI's documentation only stipulates that Initialize()
                             * "should perform all tasks that are to take place before entering the model’s time loop," so
                             * it seems legal to e.g. call GetInputVarNames() before Initialize()--which would require the
                             * backing model to already exist--so creating it in Initialize() is not an option.
                             */
                        }
                    );
                }
                catch (const ::external::ExternalIntegrationException &e) {
                    // "Override" the default message in this case
                    this->init_exception_msg =
                            "Cannot init " + this->model_name + ", failed to acquire creator and/or destroyer functions: " +
                            this->init_exception_msg;
                    throw ::external::ExternalIntegrationException(this->init_exception_msg);
                }
            }

            /**
             * Get model time step size pointer, using lazy loading when fixed.
             *
             * Get a pointer to the value of the backing model's time step size.  If the model is configured to have
             * fixed time step size, return the pointer from `bmi_model_time_step_size`, potentially after it is lazily
             * loaded.  If the model's time step is not fixed, retrieve it directly from the model in real time, and do
             * not set `bmi_model_time_step_size`.
             *
             * @return A pointer to the value of the backing model's time step size.
             * @see retrieve_bmi_model_time_step_size
             */
            inline std::shared_ptr<double> get_bmi_model_time_step_size_ptr() {
                if (bmi_model_time_step_size == nullptr) {
                    if (!bmi_model_has_fixed_time_step)
                        return retrieve_bmi_model_time_step_size();
                    else
                        bmi_model_time_step_size = retrieve_bmi_model_time_step_size();
                }
                return bmi_model_time_step_size;
            }

            /**
             * Retrieve time step size directly from model.
             *
             * Retrieve the time step size from the backing model and return in a shared pointer.
             *
             * @return Shared pointer to just-retrieved time step size
             */
            inline std::shared_ptr<double> retrieve_bmi_model_time_step_size() {
                return std::make_shared<double>(bmi_model->GetTimeStep());
            }

        private:

            std::string model_create_fname;
            std::string model_destroy_fname;

            /**
             * Construct the backing BMI model object, then call its BMI-native ``Initialize()`` function.
             *
             * The essentially provides the functionality for @see construct_and_init_backing_model without being a
             * virtual function.
             *
             * Implementations should return immediately without taking any further action if ``model_initialized`` is
             * already ``true``.
             *
             * The call to the BMI native ``Initialize(string)`` should pass the value stored in ``bmi_init_config``.
             */
            inline void construct_and_init_backing_model_for_type() {
                if (model_initialized)
                    return;
                execModuleCreation();
                bmi_model->Initialize(bmi_init_config.c_str());
            }

            /**
             * A non-virtual equivalent for the virtual @see Finalize.
             *
             * This function should be kept private.  If its logic needs to be invoked externally, that should be done
             * via the destructor or via the public interface @see Finalize function.
             *
             * Primarily, this exists to contain the functionality appropriate for @see Finalize in a function that is
             * non-virtual, and can therefore be called by a destructor.
             */
            void finalizeForCppAdapter() {
                if (bmi_model != nullptr && model_initialized) {
                    model_initialized = false;

                    //C++ Models should throw their own exceptions...do we want to wrap in State_Exception? 
                    // It looks like the Python adapter just marshals native exceptions, so guessing not.
                    bmi_model->Finalize();

                    // Acquire and call the BMI destroyer function
                    ModelDestroyer dynamic_destroyer;
                    void* symbol = dynamic_load_symbol(model_destroy_fname);
                    dynamic_destroyer = (ModelDestroyer) symbol;
                    dynamic_destroyer(this->bmi_model.get());
                }
           }

            // For unit testing
            friend class ::Bmi_Cpp_Adapter_Test;

        };

    }
}


#endif //NGEN_BMI_CPP_ADAPTER_H
