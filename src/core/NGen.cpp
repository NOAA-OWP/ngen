#include <iostream>
#include <memory>

#include "NGenConfig.h"
#include "Hymod.h"

int main(int argc, char* argv[]) {
    std::cout << "Hello there " << ngen_VERSION_MAJOR << "."
        << ngen_VERSION_MINOR << "."
        << ngen_VERSION_PATCH << std::endl;
}