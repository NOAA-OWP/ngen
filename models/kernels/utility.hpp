#ifndef KERNEL_UTILITY_H
#define KERNEL_UTILITY_H

#include <iostream>
#include <vector>
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/* ALL THE STUFF BELOW HERE IS JUST UTILITY MEMORY AND TIME FUNCTION CODE */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/


inline int dayofweek(double j)
{
  j += 0.5;
  return (int) (j + 1) % 7;
}

inline void itwo_alloc(std::vector<std::vector<int>> *array,int rows, int cols)
{
    array->resize(rows, std::vector<int>(cols));
}


inline void dtwo_alloc(std::vector<std::vector<double>>  *array, int rows, int cols)
{
    array->resize(rows, std::vector<double>(cols));
}

inline void d_alloc(std::vector<double> *var,int size)
{
    var->resize(size);
}

inline void i_alloc(std::vector<int> *var,int size)
{
    var->resize(size);
}

#endif // KERNEL_UTILITY_H
