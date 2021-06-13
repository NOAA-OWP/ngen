#ifdef ACTIVATE_PYTHON

#include <exception>
#include <utility>

#include "Routing_Py_Adapter.hpp"
//#include "boost/algorithm/string.hpp"

using namespace routing_py_adapter;


Routing_Py_Adapter::Routing_Py_Adapter(std::string t_route_connection_path, std::string input_path,
                                       const std::vector<std::string> &ids,
                                       const std::vector<double> &flow_vector)
{
    int a = 1;
  py::module_ sys = py::module_::import("sys");

  py::object sys_path = sys.attr("path");

  py::object sys_path_append = sys_path.attr("append");

  sys_path_append(t_route_connection_path);

  sys_path_append(input_path);

  py::module_ t_route_module = py::module_::import("next_gen_network_module");

  py::object set_paths = t_route_module.attr("set_paths");

  set_paths(t_route_connection_path);


  //this->convert_vector_to_numpy_array(flow_vector);


  //py::array_t<double, py::array::c_style | py::array::forcecast> array;

  std::cout << "flow_vector" << std::endl;

  for(int i=0; i < flow_vector.size(); i++){
    //vector[i].doSomething();
    std::cout << flow_vector[i] << std::endl;
  }


  // allocate py::array (to pass the result of the C++ function to Python)
  auto result = py::array_t<double>(flow_vector.size());
  std::cout << "result" << std::endl;
  std::cout << result << std::endl;
  auto result_buffer = result.request();
  double *result_ptr = (double *) result_buffer.ptr;

  std::cout << "result_ptr" << std::endl;
  std::cout << result_ptr << std::endl;


  // copy std::vector -> py::array
  std::memcpy(result_ptr, flow_vector.data(), flow_vector.size()*sizeof(double));

  
  std::cout << "*result_ptr" << std::endl;
  std::cout << *(result_ptr + 3) << std::endl;
  std::cout << *(result_ptr + 3) << std::endl;

  /*
  for(int i=0; i < result.size(); i++){
    //vector[i].doSomething();
    std::cout << result[i] << std::endl;
  }
  */

  py::print("result print");
  py::print(len(result));

  py::object receive_flow_values = t_route_module.attr("receive_flow_values");

  receive_flow_values(result);


}


void Routing_Py_Adapter::convert_vector_to_numpy_array(const std::vector<double> &flow_vector)
//convert_vector_to_numpy_array(double flow_vector)
{
  int b = 1;

  py::array_t<double, py::array::c_style | py::array::forcecast> array;


  // allocate py::array (to pass the result of the C++ function to Python)
  auto result = py::array_t<int>(array.size());
  auto result_buffer = result.request();
  int *result_ptr = (int *) result_buffer.ptr;

  // copy std::vector -> py::array
  std::memcpy(result_ptr, flow_vector.data(), flow_vector.size()*sizeof(double));

  //return result;
}





#endif //ACTIVATE_PYTHON
