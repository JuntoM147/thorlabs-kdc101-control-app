cmake_minimum_required(VERSION 3.19)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(confo_quanta_algo LANGUAGES CXX)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Gui)

add_library(algo_conversion STATIC convert/conversion.cc convert/conversion.h)
target_compile_features(algo_conversion PUBLIC cxx_std_23)
target_include_directories(algo_conversion PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(algo_conversion PUBLIC Qt6::Gui)

add_library(algo_algorithm STATIC algorithm/algorithm.cc algorithm/algorithm.h instructions/instructions.h)
target_link_libraries(algo_algorithm PUBLIC algo_conversion)

include(CTest)
if(BUILD_TESTING)
    add_executable(conversion_test ../../tests/algo/conversion_test.cc)
    target_link_libraries(conversion_test PRIVATE algo_conversion)
    add_test(NAME conversion_test COMMAND conversion_test)
    add_executable(algorithm_test ../../tests/algo/algorithm_test.cc)
    target_link_libraries(algorithm_test PRIVATE algo_algorithm)
    add_test(NAME algorithm_test COMMAND algorithm_test)
    set_tests_properties(conversion_test algorithm_test PROPERTIES TIMEOUT 30)
    if(WIN32)
        foreach(test_target IN ITEMS conversion_test algorithm_test)
            add_custom_command(TARGET ${test_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Qt6::Core> $<TARGET_FILE:Qt6::Gui>
                    $<TARGET_FILE_DIR:${test_target}>
                VERBATIM)
        endforeach()
    endif()
endif()
