#ifndef TEST_BMI_CPP_H
#define TEST_BMI_CPP_H

#include <memory>
#include <string>
#include <vector>
#include <map>
#include "extern/bmi-cxx/bmi.hxx"

#define TRUE 1
#define FALSE 0
#define DEBUG 1

#define DEFAULT_TIME_STEP_SIZE 3600
#define DEFAULT_TIME_STEP_COUNT 24

#define INPUT_VAR_NAME_COUNT 2
#define OUTPUT_VAR_NAME_COUNT 2

#define BMI_TYPE_NAME_DOUBLE "double"
#define BMI_TYPE_NAME_FLOAT "float"
#define BMI_TYPE_NAME_INT "int"
#define BMI_TYPE_NAME_SHORT "short"
#define BMI_TYPE_NAME_LONG "long"

class TestBmiCpp : public bmi::Bmi {
    public:
        /**
        * Create a new model data struct instance, allocating memory for the struct itself but not any pointers within it.
        *
        * The ``time_step_size`` member is set to a defined default.  All other members are set to ``0`` or ``NULL`` (for
        * pointers).
        *
        * @return Pointer to the newly created @ref test_bmi_c_model struct instance in memory.
        */
        TestBmiCpp(){};


        virtual void Initialize(std::string config_file);
        virtual void Update();
        virtual void UpdateUntil(double time);
        virtual void Finalize();

        // Model information functions.
        virtual std::string GetComponentName();
        virtual int GetInputItemCount();
        virtual int GetOutputItemCount();
        virtual std::vector<std::string> GetInputVarNames();
        virtual std::vector<std::string> GetOutputVarNames();

        // Variable information functions
        virtual int GetVarGrid(std::string name);
        virtual std::string GetVarType(std::string name);
        virtual std::string GetVarUnits(std::string name);
        virtual int GetVarItemsize(std::string name);
        virtual int GetVarNbytes(std::string name);
        virtual std::string GetVarLocation(std::string name);

        virtual double GetCurrentTime();
        virtual double GetStartTime();
        virtual double GetEndTime();
        virtual std::string GetTimeUnits();
        virtual double GetTimeStep();

        // Variable getters
        virtual void GetValue(std::string name, void *dest);
        virtual void *GetValuePtr(std::string name);
        virtual void GetValueAtIndices(std::string name, void *dest, int *inds, int count);

        // Variable setters
        virtual void SetValue(std::string name, void *src);
        virtual void SetValueAtIndices(std::string name, int *inds, int count, void *src);

        // Grid information functions
        virtual int GetGridRank(const int grid);
        virtual int GetGridSize(const int grid);
        virtual std::string GetGridType(const int grid);

        virtual void GetGridShape(const int grid, int *shape);
        virtual void GetGridSpacing(const int grid, double *spacing);
        virtual void GetGridOrigin(const int grid, double *origin);

        virtual void GetGridX(int grid, double *x);
        virtual void GetGridY(const int grid, double *y);
        virtual void GetGridZ(const int grid, double *z);

        virtual int GetGridNodeCount(const int grid);
        virtual int GetGridEdgeCount(const int grid);
        virtual int GetGridFaceCount(const int grid);

        virtual void GetGridEdgeNodes(const int grid, int *edge_nodes);
        virtual void GetGridFaceEdges(const int grid, int *face_edges);
        virtual void GetGridFaceNodes(const int grid, int *face_nodes);
        virtual void GetGridNodesPerFace(const int grid, int *nodes_per_face);



    private:
        static const int input_var_name_count = INPUT_VAR_NAME_COUNT;
        static const int output_var_name_count = OUTPUT_VAR_NAME_COUNT;

        std::vector<std::string> input_var_names = { "INPUT_VAR_1", "INPUT_VAR_2" };
        std::vector<std::string> output_var_names = { "OUTPUT_VAR_1", "OUTPUT_VAR_2" };
        std::vector<std::string> input_var_types = { "double", "double" };
        std::vector<std::string> output_var_types = { "double", "double" };
        std::vector<std::string> input_var_units = { "m", "m" };
        std::vector<std::string> output_var_units = { "m", "m/s" };
        std::vector<std::string> input_var_locations = { "node", "node" };
        std::vector<std::string> output_var_locations = { "node", "node" };

        int input_var_item_count[INPUT_VAR_NAME_COUNT] = { 1, 1 };
        int output_var_item_count[OUTPUT_VAR_NAME_COUNT] = { 1, 1 };
        int input_var_grids[INPUT_VAR_NAME_COUNT] = { 1, 1 };
        int output_var_grids[OUTPUT_VAR_NAME_COUNT] = { 1, 1 };

        std::map<std::string,int> type_sizes = {
            {BMI_TYPE_NAME_DOUBLE, sizeof(double)},
            {BMI_TYPE_NAME_FLOAT, sizeof(float)},
            {BMI_TYPE_NAME_INT, sizeof(int)},
            {BMI_TYPE_NAME_SHORT, sizeof(short)},
            {BMI_TYPE_NAME_LONG, sizeof(long)}
        };

        // ***********************************************************
        // ***************** Non-dynamic allocations *****************
        // ***********************************************************

        // Epoch-based start time (BMI start time is considered 0.0)
        long epoch_start_time = 0.0;
        int num_time_steps = 0;
        double current_model_time = 0.0;
        double model_end_time = 0.0;
        int time_step_size = DEFAULT_TIME_STEP_SIZE;

        // ***********************************************************
        // ******************* Dynamic allocations *******************
        // ***********************************************************
        std::unique_ptr<double> input_var_1 = nullptr;
        std::unique_ptr<double> input_var_2 = nullptr;
        std::unique_ptr<double> output_var_1 = nullptr;
        std::unique_ptr<double> output_var_2 = nullptr;

        /**
        * Read the BMI initialization config file and use its contents to set the state of the model.
        *
        * @param config_file The path to the config file.
        * @param model Pointer to the model struct instance.
        * @return The BMI return code indicating success or failure as appopriate.
        */
        void read_init_config(std::string config_file);

        /**
         * Read number of lines in file and max line length, returning -1 if it does not exist or could not be read.
         *
         * @param file_name The name of the file to open and read.
         * @param line_count A pointer to a location in which to write the value for the number of lines in the file.
         * @param max_line_length A pointer to a location in which to write the value of the max line length for the file.
         * @return 0 if successful or -1 otherwise.
         */
        void read_file_line_counts(std::string file_name, int* line_count, int* max_line_length);

        /**
         * Run this model into the future.
         *
         * @param model The model struct instance.
         * @param dt The number of seconds into the future to advance the model.
         * @return 0 if successful or 1 otherwise.
         */
        void run(long dt);

};

extern "C"
{
    /**
    * Construct this BMI instance as a normal C++ object, to be returned to the framework.
    *
    * @return A pointer to the newly allocated instance.
    */
	TestBmiCpp *bmi_model_create()
	{
		return new TestBmiCpp();
	}

    /**
     * @brief Destroy/free an instance created with @see bmi_model_create
     * 
     * @param ptr 
     */
	void bmi_model_destroy(TestBmiCpp *ptr)
	{
		delete ptr;
	}
}

#endif //TEST_BMI_CPP_H
