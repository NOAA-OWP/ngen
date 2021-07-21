#ifndef BMI_TEST_BMI_C_H
#define BMI_TEST_BMI_C_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "bmi.h"
#include "test_bmi_c.h"

    /**
     * Read number of lines in file and max line length, returning -1 if it does not exist or could not be read.
     *
     * @param file_name The name of the file to open and read.
     * @param line_count A pointer to a location in which to write the value for the number of lines in the file.
     * @param max_line_length A pointer to a location in which to write the value of the max line length for the file.
     * @return 0 if successful or -1 otherwise.
     */
    int read_file_line_counts(const char* file_name, int* line_count, int* max_line_length);

   /**
    * Create a new model data struct instance, allocating memory for the struct itself but not any pointers within it.
    *
    * The ``time_step_size`` member is set to a defined default.  All other members are set to ``0`` or ``NULL`` (for
    * pointers).
    *
    * @return Pointer to the newly created @ref test_bmi_c_model struct instance in memory.
    */
    test_bmi_c_model * new_bmi_model(void);

   /**
    * Read the BMI initialization config file and use its contents to set the state of the model.
    *
    * @param config_file The path to the config file.
    * @param model Pointer to the model struct instance.
    * @return The BMI return code indicating success or failure as appopriate.
    */
    int read_init_config(const char* config_file, test_bmi_c_model* model);

   /**
    * Construct this BMI instance, creating the backing data struct and setting required function pointers.
    *
    * Function first creates a new data structure struct (i.e., @ref test_bmi_c_model) for the BMI instance via
    * @ref new_bmi_model, assigning the returned pointer to the BMI instance's ``data`` member.
    *
    * The function then sets all the BMI instance's function pointers, essentially "registering" other functions known
    * here so they can be accessible externally via this BMI instance.  The result is that the struct can then be used
    * much like a typical object from OO languages.
    *
    * @param model A pointer to the @ref Bmi instance to register/construct.
    * @return A pointer to the passed-in @ref Bmi instance.
    */
    Bmi* register_bmi(Bmi *model);

#if defined(__cplusplus)
}
#endif

#endif //BMI_TEST_BMI_C_H
