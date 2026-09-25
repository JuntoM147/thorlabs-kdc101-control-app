cmake_minimum_required(VERSION 3.19)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Open the repository-root CMakeLists.txt to build the complete application.")
endif()

add_library(application STATIC
    application/application.cc application/application.h
    application_types/application_types.h
    motor_controller/motor_controller.cc motor_controller/motor_controller.h
    laser_controller/laser_controller.cc laser_controller/laser_controller.h
    motor_worker/motor_worker.cc motor_worker/motor_worker.h
    laser_worker/laser_worker.cc laser_worker/laser_worker.h
    scan_controller/scan_controller.cc scan_controller/scan_controller.h)
set_target_properties(application PROPERTIES AUTOMOC ON)
target_compile_features(application PUBLIC cxx_std_23)
target_include_directories(application PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(application PUBLIC Qt6::Core algo_algorithm kinesis_devices ni_daq_laser)

if(BUILD_TESTING AND DEVICES_USE_FAKE_KINESIS AND DEVICES_USE_FAKE_NI_DAQ)
    add_executable(application_test ../../tests/application/application_test.cc)
    set_target_properties(application_test PROPERTIES AUTOMOC ON)
    target_include_directories(application_test PRIVATE ../../tests/kdc101/fake_sdk)
    target_link_libraries(application_test PRIVATE application Qt6::Test)
    add_test(NAME application_test COMMAND application_test)
    set_tests_properties(application_test PROPERTIES TIMEOUT 45)
    if(WIN32)
        add_custom_command(TARGET application_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Core> $<TARGET_FILE:Qt6::Gui> $<TARGET_FILE:Qt6::Test>
                $<TARGET_FILE_DIR:application_test> VERBATIM)
    endif()
endif()
