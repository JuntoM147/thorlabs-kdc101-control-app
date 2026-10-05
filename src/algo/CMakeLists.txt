add_library(algo_conversion STATIC convert/conversion.cc convert/conversion.h)
target_include_directories(algo_conversion PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(algo_conversion PUBLIC Qt6::Gui)

add_library(algo_algorithm STATIC
    algorithm/algorithm.cc
    algorithm/algorithm.h
    instructions/instructions.h
)
target_link_libraries(algo_algorithm PUBLIC algo_conversion)
