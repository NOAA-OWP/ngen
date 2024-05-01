#ifndef __OUTPUT_INTERFACE_HPP__
#define __OUTPUT_INTERFACE_HPP__

#include <vector>
#include <string>
namespace realization 
{
    /**
     * @brief Interface showing required methods to provide data from a forumulation for output
     * 
     * This interface exposes the minimum needed functionality from BMI to allow extraction of output values for transfer to framework output.
    */

    class OutputInterface 
    {
        public:

        /**
         * @brief Return a list of string containing the name of all valid output variables
        */
        virtual std::vector<std::string> get_output_variable_names() = 0;

        /**
         * @brief Return a size of the indicated output variable, or a negative value for an invalid name
        */
        virtual long get_output_size(std::string var_name) = 0;    

        /**
         * @brief Return the current output value as a binary array
        */
        virtual std::vector<uint8_t> get_output_value(std::string var_name) = 0;

        /**
         * @brief Return the type of an output variable as a sting, or "invalid" for an invalid name
        */
        virtual std::string get_output_type(std::string var_name) = 0;       
    
        /**
         * @brief Return the X size of the indicated grid
        */
        virtual double get_grid_x(const int grid) = 0;     
    
        /**
         * @brief Return the Y size of the indicated grid
        */
        virtual double get_grid_y(const int grid) = 0;  
    
        /**
         * @brief Return the Z size of the indicated grid
        */
        virtual double get_grid_z(const int grid) = 0;      
    };
}
#endif