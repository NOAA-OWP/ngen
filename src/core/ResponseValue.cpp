#include "ResponseValue.hpp"
#include <stdexcept>
#include <assert.h>

using namespace responsevalue;

double ResponseValue::get_value(const std::string& name){
    _ValueData& d = value_data[name];
    if (d.units_match)
        return d.value;
    else
        return handle_unit_conversion(d.input_units, d.output_units, d.value);
}

double ResponseValue::get_primary(){
    return get_value(primary_name);
}

std::string ResponseValue::get_primary_name(){
    return primary_name;
}

std::string ResponseValue::get_input_var_units(const std::string& name){
    return value_data[name].input_units;
}

std::string ResponseValue::get_output_var_units(const std::string& name){
    return value_data[name].output_units;
}

void ResponseValue::set_value(const std::string& name, double value){
    value_data[name].value = value;
}

void ResponseValue::set_primary(double value){
    set_value(primary_name, value);
}

void ResponseValue::set_primary_name(const std::string& name){
    primary_name = name;
}

void ResponseValue::set_input_var_units(const std::string& name, const std::string& units){
    _ValueData& d = value_data[name];
    d.input_units = units;
    //assert (d.input_units == value_data[name].input_units); //TODO: Remove me!
    if(d.input_units != d.output_units){
        d.units_match = false;
    }
}

void ResponseValue::set_output_var_units(const std::string& name, const std::string& units){
    _ValueData& d = value_data[name];
    d.output_units = units;
    //assert (d.output_units == value_data[name].output_units); //TODO: Remove me!
    if(d.input_units != d.output_units){
        d.units_match = false;
    }
}

double ResponseValue::handle_unit_conversion(const std::string& input_units, const std::string& output_units, double value){
    throw std::runtime_error("Not Yet Implemented. (ResponseValue unit conversion)");
}

