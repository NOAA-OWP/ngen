#ifndef __OUTPUT_INTERFACE_HPP__
#define __OUTPUT_INTERFACE_HPP__

#include <vector>
#include <string>

/**
 * @brief Interface showing required methods to provide data from a forumulation for output
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
     * @brief Return a pointer to the current value of an output variable, or NULL for an invalid name
    */
    virtual void* get_output_value(std::string var_name) = 0;

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

#endif