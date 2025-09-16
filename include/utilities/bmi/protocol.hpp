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
Virtual interface for BMI protocols
*/

#pragma once

#include "Bmi_Adapter.hpp"
#include "JSONProperty.hpp"


namespace models{ namespace bmi{ namespace protocols{

struct Context{
    const int current_time_step;
    const int total_steps;
    const std::string& timestamp;
    const std::string& id;
};

class NgenBmiProtocol{
  /**
   * @brief Abstract interface for a generic BMI protocol
   * 
   */

  public:
    /**
     * @brief Run the BMI protocol
     * 
     */
    virtual void run(const Context& ctx) const = 0;

    virtual ~NgenBmiProtocol() = default;

  private:
    /**
     * @brief Check if the BMI protocol is supported by the model
     * 
     */
    virtual void check_support() = 0;
    /**
     * @brief Initialize the BMI protocol from a set of key/value properties
     * 
     * @param properties 
     */
    virtual void initialize(const geojson::PropertyMap& properties) = 0;

  protected:
    
    /**
     * @brief Constructor for subclasses to create NgenBmiProtocol objects
     * 
     * @param model A shared pointer to a Bmi_Adapter object which should be
     * initialized before being passed to this constructor.
     */
    NgenBmiProtocol(std::shared_ptr<models::bmi::Bmi_Adapter> model): model(model){}

    /**
     * @brief The Bmi_Adapter object used by the protocol
     * 
     */
    std::shared_ptr<models::bmi::Bmi_Adapter> model;

    /**
     * @brief Whether the protocol is supported by the model
     * 
     */
    bool is_supported = false;
}; 

}}}
