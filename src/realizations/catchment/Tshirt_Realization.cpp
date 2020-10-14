#include "giuh_kernel.hpp"
#include "Tshirt_Realization.hpp"
#include "TshirtErrorCodes.h"
#include "Catchment_Formulation.hpp"
using namespace realization;

Tshirt_Realization::Tshirt_Realization(
        forcing_params forcing_config,
        utils::StreamHandler output_stream,
        double soil_storage_meters,
        double groundwater_storage_meters,
        std::string catchment_id,
        giuh::GiuhJsonReader &giuh_json_reader,
        tshirt::tshirt_params params,
        const vector<double> &nash_storage,
        time_step_t t)
    : Catchment_Formulation(catchment_id, forcing_config, output_stream), catchment_id(catchment_id), dt(t)
{
    this->params = &params;
    giuh_kernel = giuh_json_reader.get_giuh_kernel_for_id(this->catchment_id);

    // If the look-up failed in the reader for some reason, and we got back a null pointer ...
    if (this->giuh_kernel == nullptr) {
        // ... revert to a pass-through kernel
        this->giuh_kernel = std::make_shared<giuh::giuh_kernel>(
                giuh::giuh_kernel(this->catchment_id, giuh_json_reader.get_associated_comid(this->catchment_id)));
    }

    //FIXME not really used, don't call???
    //add_time(t, params.nash_n);
    state[0] = std::make_shared<tshirt::tshirt_state>(tshirt::tshirt_state(soil_storage_meters, groundwater_storage_meters, nash_storage));
    //state[0]->soil_storage_meters = soil_storage_meters;
    //state[0]->groundwater_storage_meters = groundwater_storage_meters;

    for (int i = 0; i < params.nash_n; ++i) {

        state[0]->nash_cascade_storeage_meters[i] = nash_storage[i];
    }

    model = make_unique<tshirt::tshirt_model>(tshirt::tshirt_model(params, state[0]));
}

Tshirt_Realization::Tshirt_Realization(
        forcing_params forcing_config,
        utils::StreamHandler output_stream,
        double soil_storage_meters,
        double groundwater_storage_meters,
        std::string catchment_id,
        giuh::GiuhJsonReader &giuh_json_reader,
        double maxsmc,
        double wltsmc,
        double satdk,
        double satpsi,
        double slope,
        double b,
        double multiplier,
        double alpha_fc,
        double Klf,
        double Kn,
        int nash_n,
        double Cgw,
        double expon,
        double max_gw_storage,
        const std::vector<double> &nash_storage,
        time_step_t t
) : Tshirt_Realization::Tshirt_Realization(forcing_config, output_stream, soil_storage_meters, groundwater_storage_meters,
                                           catchment_id, giuh_json_reader,
                                           tshirt::tshirt_params(maxsmc, wltsmc, satdk, satpsi, slope, b, multiplier,
                                                                 alpha_fc, Klf, Kn, nash_n, Cgw, expon, max_gw_storage),
                                           nash_storage, t) {

}

double Tshirt_Realization::get_response(double input_flux, time_step_t t, const shared_ptr<pdm03_struct>& et_params) {
    //FIXME doesn't do anything, don't call???
    //add_time(t+1, params.nash_n);
    double precip = this->forcing.get_next_hourly_precipitation_meters_per_second();
    //FIXME should this run "daily" or hourly (t) which should really be dt
    //Do we keep an "internal dt" i.e. this->dt and reconcile with t?
    int error = model->run(t, precip*dt/1000, et_params);
    if(error == tshirt::TSHIRT_MASS_BALANCE_ERROR){
      std::cout<<"WARNING Tshirt_Realization::model mass balance error"<<std::endl;
    }
    state[t+1] = model->get_current_state();
    fluxes[t] = model->get_fluxes();
    double giuh = giuh_kernel->calc_giuh_output(t, fluxes[t]->surface_runoff_meters_per_second);
    return fluxes[t]->soil_lateral_flow_meters_per_second + fluxes[t]->groundwater_flow_meters_per_second +
           giuh;
}

void Tshirt_Realization::create_formulation(geojson::PropertyMap properties) {
    this->validate_parameters(properties);

    this->catchment_id = this->get_id();
    this->dt = properties.at("timestep").as_natural_number();

    tshirt::tshirt_params tshirt_params{
        properties.at("maxsmc").as_real_number(),   //maxsmc FWRFH
        properties.at("wltsmc").as_real_number(),  //wltsmc  from fred_t-shirt.c FIXME NOT USED IN TSHIRT?!?!
        properties.at("satdk").as_real_number(),   //satdk FWRFH
        properties.at("satpsi").as_real_number(),    //satpsi    FIXME what is this and what should its value be?
        properties.at("slope").as_real_number(),   //slope
        properties.at("scaled_distribution_fn_shape_parameter").as_real_number(),      //b bexp? FWRFH
        properties.at("multiplier").as_real_number(),    //multipier  FIXMME (lksatfac)
        properties.at("alpha_fc").as_real_number(),    //aplha_fc   field_capacity_atm_press_fraction
        properties.at("Klf").as_real_number(),    //Klf lateral flow nash coefficient?
        properties.at("Kn").as_real_number(),    //Kn Kn	0.001-0.03 F Nash Cascade coeeficient
        static_cast<int>(properties.at("nash_n").as_natural_number()),      //number_lateral_flow_nash_reservoirs
        properties.at("Cgw").as_real_number(),    //fred_t-shirt gw res coeeficient (per h)
        properties.at("expon").as_real_number(),    //expon FWRFH
        properties.at("max_groundwater_storage_meters").as_real_number()   //max_gw_storage Sgwmax FWRFH
    };

    this->params = &tshirt_params;

    double soil_storage_meters = tshirt_params.max_soil_storage_meters * properties.at("soil_storage_percentage").as_real_number();
    double ground_water_storage = tshirt_params.max_groundwater_storage_meters * properties.at("groundwater_storage_percentage").as_real_number();

    std::vector<double> nash_storage = properties.at("nash_storage").as_real_vector();

    this->state[0] = std::make_shared<tshirt::tshirt_state>(tshirt::tshirt_state(soil_storage_meters, ground_water_storage, nash_storage));

    for (int i = 0; i < tshirt_params.nash_n; ++i) {

        this->state[0]->nash_cascade_storeage_meters[i] = nash_storage[i];
    }

    this->model = make_unique<tshirt::tshirt_model>(tshirt::tshirt_model(tshirt_params, this->state[0]));

    geojson::JSONProperty giuh = properties.at("giuh");

    std::vector<std::string> missing_parameters;

    if (!giuh.has_key("giuh_path")) {
        missing_parameters.push_back("giuh_path");
    }

    if (!giuh.has_key("crosswalk_path")) {
        missing_parameters.push_back("crosswalk_path");
    }

    if (missing_parameters.size() > 0) {
        std::string message = "A giuh configuration cannot be created for '" + this->get_id() + "'; the following parameters are missing: ";

        for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
            message += missing_parameters[missing_parameter_index];

            if (missing_parameter_index < missing_parameters.size() - 1) {
                message += ", ";
            }
        }
        
        throw std::runtime_error(message);
    }

    std::unique_ptr<giuh::GiuhJsonReader> giuh_reader = std::make_unique<giuh::GiuhJsonReader>(
        giuh.at("giuh_path").as_string(),
        giuh.at("crosswalk_path").as_string()
    );

    this->giuh_kernel = giuh_reader->get_giuh_kernel_for_id(this->catchment_id);

    if (this->giuh_kernel == nullptr) {
        // ... revert to a pass-through kernel
        this->giuh_kernel = std::make_shared<giuh::giuh_kernel>(
                giuh::giuh_kernel(
                    this->catchment_id,
                    giuh_reader->get_associated_comid(this->catchment_id)
                )
        );
    }
}

void Tshirt_Realization::create_formulation(boost::property_tree::ptree &config, geojson::PropertyMap *global) {
    geojson::PropertyMap options = this->interpret_parameters(config, global);

    this->catchment_id = this->get_id();
    this->dt = options.at("timestep").as_natural_number();

    tshirt::tshirt_params tshirt_params{
        options.at("maxsmc").as_real_number(),   //maxsmc FWRFH
        options.at("wltsmc").as_real_number(),  //wltsmc  from fred_t-shirt.c FIXME NOT USED IN TSHIRT?!?!
        options.at("satdk").as_real_number(),   //satdk FWRFH
        options.at("satpsi").as_real_number(),    //satpsi    FIXME what is this and what should its value be?
        options.at("slope").as_real_number(),   //slope
        options.at("scaled_distribution_fn_shape_parameter").as_real_number(),      //b bexp? FWRFH
        options.at("multiplier").as_real_number(),    //multipier  FIXMME (lksatfac)
        options.at("alpha_fc").as_real_number(),    //aplha_fc   field_capacity_atm_press_fraction
        options.at("Klf").as_real_number(),    //Klf lateral flow nash coefficient?
        options.at("Kn").as_real_number(),    //Kn Kn	0.001-0.03 F Nash Cascade coeeficient
        static_cast<int>(options.at("nash_n").as_natural_number()),      //number_lateral_flow_nash_reservoirs
        options.at("Cgw").as_real_number(),    //fred_t-shirt gw res coeeficient (per h)
        options.at("expon").as_real_number(),    //expon FWRFH
        options.at("max_groundwater_storage_meters").as_real_number()   //max_gw_storage Sgwmax FWRFH
    };

    this->params = &tshirt_params;

    double soil_storage_meters = tshirt_params.max_soil_storage_meters * options.at("soil_storage_percentage").as_real_number();
    double ground_water_storage = tshirt_params.max_groundwater_storage_meters * options.at("groundwater_storage_percentage").as_real_number();

    std::vector<double> nash_storage = options.at("nash_storage").as_real_vector();

    this->state[0] = std::make_shared<tshirt::tshirt_state>(tshirt::tshirt_state(soil_storage_meters, ground_water_storage, nash_storage));

    for (int i = 0; i < tshirt_params.nash_n; ++i) {

        this->state[0]->nash_cascade_storeage_meters[i] = nash_storage[i];
    }

    this->model = make_unique<tshirt::tshirt_model>(tshirt::tshirt_model(tshirt_params, this->state[0]));

    geojson::JSONProperty giuh = options.at("giuh");

    std::vector<std::string> missing_parameters;

    if (!giuh.has_key("giuh_path")) {
        missing_parameters.push_back("giuh_path");
    }

    if (!giuh.has_key("crosswalk_path")) {
        missing_parameters.push_back("crosswalk_path");
    }

    if (missing_parameters.size() > 0) {
        std::string message = "A giuh configuration cannot be created for '" + this->get_id() + "'; the following parameters are missing: ";

        for (int missing_parameter_index = 0; missing_parameter_index < missing_parameters.size(); missing_parameter_index++) {
            message += missing_parameters[missing_parameter_index];

            if (missing_parameter_index < missing_parameters.size() - 1) {
                message += ", ";
            }
        }
        
        throw std::runtime_error(message);
    }

    std::unique_ptr<giuh::GiuhJsonReader> giuh_reader = std::make_unique<giuh::GiuhJsonReader>(
        giuh.at("giuh_path").as_string(),
        giuh.at("crosswalk_path").as_string()
    );

    this->giuh_kernel = giuh_reader->get_giuh_kernel_for_id(this->catchment_id);

    if (this->giuh_kernel == nullptr) {
        // ... revert to a pass-through kernel
        this->giuh_kernel = std::make_shared<giuh::giuh_kernel>(
                giuh::giuh_kernel(
                    this->catchment_id,
                    giuh_reader->get_associated_comid(this->catchment_id)
                )
        );
    }
}

void Tshirt_Realization::set_giuh_kernel(std::shared_ptr<giuh::GiuhJsonReader> reader) {
    this->giuh_kernel = reader->get_giuh_kernel_for_id(this->catchment_id);

    // If the look-up failed in the reader for some reason, and we got back a null pointer ...
    if (this->giuh_kernel == nullptr) {
        // ... revert to a pass-through kernel
        this->giuh_kernel = std::make_shared<giuh::giuh_kernel>(
                giuh::giuh_kernel(
                    this->catchment_id,
                    reader->get_associated_comid(this->catchment_id)
                )
        );
    }
}