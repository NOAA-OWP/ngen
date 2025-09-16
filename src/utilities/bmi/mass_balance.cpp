/*
Author: Nels Frazier
Copyright (C) 2025 Lynker
------------------------------------------------------------------------
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
------------------------------------------------------------------------

Version 0.1
Implementation the BMI mass balance checking protocol
*/

#include "mass_balance.hpp"

namespace models { namespace bmi { namespace protocols {

void NgenMassBalance::run(const Context& ctx) const{
    bool check_step = false;
    //if frequency was set to -1, only check at the end
    if( frequency == -1 ){
        if(ctx.current_time_step == ctx.total_steps){
            check_step = true;
        }
    }
    else{
        check_step = (ctx.current_time_step % frequency) == 0;
    }

    if(model && is_supported && check && check_step) {
        double mass_in, mass_out, mass_stored, mass_leaked, mass_balance;
        model->GetValue(INPUT_MASS_NAME, &mass_in);
        model->GetValue(OUTPUT_MASS_NAME, &mass_out);
        model->GetValue(STORED_MASS_NAME, &mass_stored);
        model->GetValue(LEAKED_MASS_NAME, &mass_leaked);
        // TODO consider unit conversion if/when it becomes necessary
        mass_balance = mass_in - mass_out - mass_stored - mass_leaked;
        if ( std::abs(mass_balance) > tolerance ) {
            std::stringstream ss;
            ss << "at timestep " << std::to_string(ctx.current_time_step)
               << " ("+ctx.timestamp+")"
               << " at feature id " << ctx.id <<std::endl
               << "\tMass balance check failed for " << model->GetComponentName() << "\n\t" <<
                INPUT_MASS_NAME << "(" << mass_in <<  ") - " <<
                OUTPUT_MASS_NAME << " (" << mass_out <<  ") - " << 
                STORED_MASS_NAME << " (" << mass_stored <<  ") - " << 
                LEAKED_MASS_NAME << " (" << mass_leaked << ") = " << 
                mass_balance << "\n\t" << "tolerance: " << tolerance << std::endl;
            if(is_fatal)
                throw MassBalanceError("mass_balance::error " + ss.str());
            else{
                std::cerr << "mass_balance::warning " + ss.str();
            }
                
        }
    }
}

void NgenMassBalance::check_support() {
    if (model->is_model_initialized()) {
        double mass_var;
        std::vector<std::string> units;
        units.reserve(4);
        try{
            for(const auto& name : 
                {INPUT_MASS_NAME, OUTPUT_MASS_NAME, STORED_MASS_NAME, LEAKED_MASS_NAME}
            ){
                model->GetValue(name, &mass_var);
                units.push_back( model->GetVarUnits(name) );
            }
            //Compare all other units to the first one (+1)
            if( std::equal( units.begin()+1, units.end(), units.begin() ) ) {
                this->is_supported = true;
            }
            else{
                // It may be possible to do unit conversion and still do meaninful mass balance
                // this could be added as an extended feature, but for now, I don't think this is
                // worth the complexity.  It is, however, worth the sanity check performed here
                // to ensure the units are consistent.
                throw MassBalanceError( "mass balance variables have incosistent units, cannot perform mass balance" );
            }
        } catch (const std::exception &e) {
            std::stringstream ss;
            ss << "mass_balance::integration: Error getting mass balance values for module '" << model->GetComponentName() << "': " << e.what() << std::endl;
            std::cout << ss.str();
            this->is_supported= false;
        }
    } else {
        throw std::runtime_error("Cannot check mass balance for uninitialized model.");
    }
}

void NgenMassBalance::initialize(const geojson::PropertyMap& properties)
{
    //Ensure the model is capable of mass balance using the protocol
    check_support();

    //now check if the user has requested to use mass balance
    auto protocol_it = properties.find(CONFIGURATION_KEY);
    if ( is_supported && protocol_it != properties.end() ) {
        geojson::PropertyMap mass_bal = protocol_it->second.get_values();

        auto _it = mass_bal.find(TOLERANCE_KEY);
        if( _it != mass_bal.end() ) tolerance = _it->second.as_real_number();

        _it = mass_bal.find(FATAL_KEY);
        if( _it != mass_bal.end() ) is_fatal = _it->second.as_boolean();

        _it = mass_bal.find(CHECK_KEY);
        if( _it != mass_bal.end() ) {
            check = _it->second.as_boolean();
        } else {
            //default to true if not specified
            check = true;
        }

        _it = mass_bal.find(FREQUENCY_KEY);
        if( _it != mass_bal.end() ){
            frequency = _it->second.as_natural_number();
        } else {
            frequency = 1; //default, check every timestep
        }
    } else{
        //no mass balance requested, or not supported, so don't check it
        check = false;
    }
}

}}} // end namespace models::bmi::protocols