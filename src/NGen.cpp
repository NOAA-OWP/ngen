#include <iostream>

#include "NGenConfig.h"
#include <Simple_Lumped_Model_Realization.hpp>
#include <HY_PointHydroNexus.hpp>

int main(int argc, char *argv[]) {
    std::cout << "Hello there " << ngen_VERSION_MAJOR << "."
              << ngen_VERSION_MINOR << "."
              << ngen_VERSION_PATCH << std::endl;


    //Create a nexus
    HY_PointHydroNexus* nexus_1 = new HY_PointHydroNexus(1, "nex-1", 1);

    //Mock up some catchment parameters
    double storage = 1.0;
    double max_storage = 1000.0;
    double a = 1.0;
    double b = 10.0;
    double Ks = 0.1;
    double Kq = 0.01;
    long n = 3;
    double t = 0;
    std::vector<double> sr_tmp = {1.0, 1.0, 1.0};

    //Create the realization
    Simple_Lumped_Model_Realization* catchment_1 = new Simple_Lumped_Model_Realization(storage, max_storage,
                                                                                       a, b, Ks, Kq, n, sr_tmp, t);
    //et_struct
    // create the struct used for ET
    pdm03_struct pdm_et_data;
    pdm_et_data.B = 1.3;
    pdm_et_data.Kv = 0.99;
    pdm_et_data.modelDay = 0.0;
    pdm_et_data.Huz = 400.0;
    pdm_et_data.Cpar = pdm_et_data.Huz / (1.0+pdm_et_data.B);

    //Run one time step
    double water = catchment_1->get_response(0, 0, &pdm_et_data);
    std::cout<<"Response of cat-1: "<<water<<std::endl;
    //nexus_1->add_upstream_flow(water, "cat-1", 0);
    nexus_1->add_upstream_flow(water, 1, 0);
    std::cout<<"nexus_1 has "<<nexus_1->get_downstream_flow(1, 0, 100.0)<<std::endl;
    
    delete catchment_1;
    delete nexus_1;
}
