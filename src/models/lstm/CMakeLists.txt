cmake_minimum_required(VERSION 3.10)
add_library(models_lstm STATIC
        LSTM.cpp)
add_library(NGen::models_lstm ALIAS models_lstm)
target_include_directories(models_lstm PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/models/lstm/include
        #${TORCH_INCLUDE_DIRS}
        )

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

target_link_libraries(models_lstm PUBLIC
        Boost::boost                # Headers-only Boost
        )
        #${TORCH_LIBRARIES})


if(LSTM_TORCH_LIB_ACTIVE)
#    find_package(Torch REQUIRED)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")
#    add_subdirectory("src/models/lstm")
#    target_link_libraries(ngen PUBLIC
#            NGen::models_lstm
#            ${TORCH_LIBRARIES}
#           )

     target_include_directories(models_lstm PUBLIC
        ${TORCH_INCLUDE_DIRS})


     target_link_libraries(models_lstm PUBLIC
        ${TORCH_LIBRARIES})
     

endif()
