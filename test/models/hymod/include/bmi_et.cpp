#include <stdio.h>

#include <string>
#include "EtCalcFunction.hpp"
#include "bmi_et.hpp"
#include <bmi.h>

void BmiEt::
Initialize (std:: string config_file)
{
  if ( config_file.compare("") != 0 )
    this->_model = et:Et(config_file);
}

void BmiEt::
Update()
{
  this->_model.advance_in_time();
}

void BmiEt::
UpdateUntil(double t)
{
  double time;
  double dt;

  time = this->GetCurrentTime();
  dt = this->GetTimeStep();
  {
    double n_steps = (t - time) / dt;
    double frac;

    for (int n=0; n<int(n_steps); n++)
      this->Update();
    
    frac = n_steps - int(n_steps);
    this->_model.dt = frac * dt;
    this->_model.advance_in_time();
    this->_model.dt = dt;

  }
}

void BmiEt::
Finalize()
{
  this->_model.~Et();
}

int BmiEt::
GetVarGrid(std::string name)
{
  return -1;
}

std::string BmiEt::
GetVarType(std::string name)
{
  if (name.compare("instantaneous_et_rate_m_per_s") == 0)
    return "double";
  else if (name.compare("psychrometric_constant_Pa_per_C") == 0)
    return "double";
  else if (name.compare("slope_sat_vap_press_curve_Pa_s") == 0)
    return "double";
  else if (name.compare("air_saturation_vapor_pressure_Pa") == 0)
    return "double";
  else if (name.compare("air_actual_vapor_pressure_Pa") == 0)
    return "double";
  else if (name.compare("moist_air_density_kg_per_m3") == 0)
    return "double";
  else if (name.compare("water_latent_heat_of_vaporization_J_per_kg") == 0)
    return "double";
  else if (name.compare("moist_air_gas_constant_J_per_kg_K") == 0)
    return "double";
  else if (name.compare("moist_air_specific_humidity_kg_per_m3") == 0)
    return "double";
  else if (name.compare("vapor_pressure_deficit_Pa") == 0)
    return "double";
  else if (name.compare("liquid_water_density_kg_per_m3") == 0)
    return "double";
  else if (name.compare("lambda_et") == 0)
    return "double";
  else if (name.compare("delta") == 0)
    return "double";
  else if (name.compare("gamma") == 0)
    return "double";
  else
    return 0;
}

int BmiEt::
GetVarItemsize(std::string name)
{
  if (name.compare("instantaneous_et_rate_m_per_s") == 0)
    return sizeof(double);
  else if (name.compare("psychrometric_constant_Pa_per_C") == 0)
    return sizeof(double);
  else if (name.compare("slope_sat_vap_press_curve_Pa_s") == 0)
    return sizeof(double);
  else if (name.compare("air_saturation_vapor_pressure_Pa") == 0)
    return sizeof(double);
  else if (name.compare("air_actual_vapor_pressure_Pa") == 0)
    return sizeof(double);
  else if (name.compare("moist_air_density_kg_per_m3") == 0)
    return sizeof(double);
  else if (name.compare("water_latent_heat_of_vaporization_J_per_kg") == 0)
    return sizeof(double);
  else if (name.compare("moist_air_gas_constant_J_per_kg_K") == 0)
    return sizeof(double);
  else if (name.compare("moist_air_specific_humidity_kg_per_m3") == 0)
    return sizeof(double);
  else if (name.compare("vapor_pressure_deficit_Pa") == 0)
    return sizeof(double);
  else if (name.compare("liquid_water_density_kg_per_m3") == 0)
    return sizeof(double);
  else if (name.compare("lambda_et") == 0)
    return sizeof(double);
  else if (name.compare("delta") == 0)
    return sizeof(double);
  else if (name.compare("gamma") == 0)
    return sizeof(double);
  else
    return 0;
}

std::string BmiEt::
GetVarUnits(std::string name)
{
  if (name.compare("instantaneous_et_rate_m_per_s") == 0)
    return "m s-1";
  else if (name.compare("psychrometric_constant_Pa_per_C") == 0)
    return "Pa C-1";
  else if (name.compare("slope_sat_vap_press_curve_Pa_s") == 0)
    return "Pa";
  else if (name.compare("air_saturation_vapor_pressure_Pa") == 0)
    return "Pa";
  else if (name.compare("air_actual_vapor_pressure_Pa") == 0)
    return "Pa";
  else if (name.compare("moist_air_density_kg_per_m3") == 0)
    return "kg m-3";
  else if (name.compare("water_latent_heat_of_vaporization_J_per_kg") == 0)
    return "J kg-1";
  else if (name.compare("moist_air_gas_constant_J_per_kg_K") == 0)
    return "J kg-1 K-1";
  else if (name.compare("moist_air_specific_humidity_kg_per_m3") == 0)
    return "kg m-3";
  else if (name.compare("vapor_pressure_deficit_Pa") == 0)
    return "Pa";
  else if (name.compare("liquid_water_density_kg_per_m3") == 0)
    return "kg m-3";
  else if (name.compare("lambda_et") == 0)
    return "Unknown";
  else if (name.compare("delta") == 0)
    return "Unknown";
  else if (name.compare("gamma") == 0)
    return "Unknown";
  else
    return 0;

}

int BmiEt::
GetVarNbytes(std::string name)
{
  int itemsize;
  int gridsize;

  itemsize = this->GetVarItemsize(name);
  gridsize = this->GetGridSize(this->GetVarGrid(name));
  
  return itemsize * gridsize;
}

std::string BmiEt::
GetVarLocation(std::string name)
{
  return "";
}

void BmiEt::
GetGridShape(const int grid, int *shape)
{
  if (grid == 0) {
    shape[0] = this->_model.shape[0];
    shape[1] = this->_model.shape[1];
  }
}


void BmiEt::
GetGridSpacing (const int grid, double * spacing)
{
  if (grid == 0) {
    spacing[0] = this->_model.spacing[0];
    spacing[1] = this->_model.spacing[1];
  }
}


void BmiEt::
GetGridOrigin (const int grid, double *origin)
{
  if (grid == 0) {
    origin[0] = this->_model.origin[0];
    origin[1] = this->_model.origin[1];
  }
}


int BmiEt::
GetGridRank(const int grid)
{
  if (grid == 0)
    return 2;
  else
    return -1;
}


int BmiEt::
GetGridSize(const int grid)
{
  if (grid == 0)
    return this->_model.shape[0] * this->_model.shape[1];
  else
    return -1;
}


std::string BmiEt::
GetGridType(const int grid)
{
  if (grid == 0)
    return "uniform_rectilinear";
  else
    return "";
}


void BmiEt::
GetGridX(const int grid, double *x)
{
  throw NotImplemented();
}


void BmiEt::
GetGridY(const int grid, double *y)
{
  throw NotImplemented();
}


void BmiEt::
GetGridZ(const int grid, double *z)
{
  throw NotImplemented();
}


int BmiEt::
GetGridNodeCount(const int grid)
{
  throw NotImplemented();
}


int BmiEt::
GetGridEdgeCount(const int grid)
{
  throw NotImplemented();
}


int BmiEt::
GetGridFaceCount(const int grid)
{
  throw NotImplemented();
}


void BmiEt::
GetGridEdgeNodes(const int grid, int *edge_nodes)
{
  throw NotImplemented();
}


void BmiEt::
GetGridFaceEdges(const int grid, int *face_edges)
{
  throw NotImplemented();
}


void BmiEt::
GetGridFaceNodes(const int grid, int *face_nodes)
{
  throw NotImplemented();
}


void BmiEt::
GetGridNodesPerFace(const int grid, int *nodes_per_face)
{
  throw NotImplemented();
}


void BmiEt::
GetValue (std::string name, void *dest)
{
  void * src = NULL;
  int nbytes = 0;

  src = this->GetValuePtr(name);
  nbytes = this->GetVarNbytes(name);

  memcpy (dest, src, nbytes);
}


void *BmiEt::
GetValuePtr (std::string name)
{
  if (name.compare("instantaneous_et_rate_m_per_s") == 0)
    return (void*)this->_model.z[0];
  else
    return NULL;
}


void BmiEt::
GetValueAtIndices (std::string name, void *dest, int *inds, int len)
{
  void * src = NULL;

  src = this->GetValuePtr(name);

  if (src) {
    int i;
    int itemsize = 0;
    int offset;
    char *ptr;

    itemsize = this->GetVarItemsize(name);

    for (i=0, ptr=(char *)dest; i<len; i++, ptr+=itemsize) {
      offset = inds[i] * itemsize;
      memcpy(ptr, (char *)src + offset, itemsize);
    }
  }
}


void BmiEt::
SetValue (std::string name, void *src)
{
  void * dest = NULL;

  dest = this->GetValuePtr(name);

  if (dest) {
    int nbytes = 0;
    nbytes = this->GetVarNbytes(name);
    memcpy(dest, src, nbytes);
  }
}


void BmiEt::
SetValueAtIndices (std::string name, int * inds, int len, void *src)
{
  void * dest = NULL;

  dest = this->GetValuePtr(name);

  if (dest) {
    int i;
    int itemsize = 0;
    int offset;
    char *ptr;

    itemsize = this->GetVarItemsize(name);

    for (i=0, ptr=(char *)src; i<len; i++, ptr+=itemsize) {
      offset = inds[i] * itemsize;
      memcpy((char *)dest + offset, ptr, itemsize);
    }
  }
}


std::string BmiHeat::
GetComponentName()
{
  return "The 2D Heat Equation";
}


int BmiHeat::
GetInputItemCount()
{
  return this->input_var_name_count;
}


int BmiHeat::
GetOutputItemCount()
{
  return this->output_var_name_count;
}


std::vector<std::string> BmiHeat::
GetInputVarNames()
{
  std::vector<std::string> names;

  for (int i=0; i<this->input_var_name_count; i++)
    names.push_back(this->input_var_names[i]);

  return names;
}


std::vector<std::string> BmiHeat::
GetOutputVarNames()
{
  std::vector<std::string> names;

  for (int i=0; i<this->input_var_name_count; i++)
    names.push_back(this->output_var_names[i]);

  return names;
}


double BmiHeat::
GetStartTime () {
  return 0.;
}


double BmiHeat::
GetEndTime () {
  return this->_model.t_end;
}


double BmiHeat::
GetCurrentTime () {
  return this->_model.time;
}


std::string BmiHeat::
GetTimeUnits() {
  return "s";
}


double BmiHeat::
GetTimeStep () {
  return this->_model.dt;
}
