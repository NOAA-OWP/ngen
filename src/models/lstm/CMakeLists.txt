cmake_minimum_required(VERSION 3.10)
add_library(models_lstm STATIC
        LSTM.cpp)
add_library(NGen::models_lstm ALIAS models_lstm)
target_include_directories(models_lstm PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/utilities
        ${PROJECT_SOURCE_DIR}/models/lstm/include
        )

target_link_libraries(models_lstm PUBLIC
        Boost::boost                # Headers-only Boost
        )

if(LSTM_TORCH_LIB_ACTIVE)
     target_include_directories(models_lstm PUBLIC
        ${TORCH_INCLUDE_DIRS})

     target_link_libraries(models_lstm PUBLIC
        ${TORCH_LIBRARIES})
endif()
