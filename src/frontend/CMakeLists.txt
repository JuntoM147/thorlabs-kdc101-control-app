cmake_minimum_required(VERSION 3.19)

# Support configuring this directory independently of the repository root.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(confo_quanta_frontend LANGUAGES CXX)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(frontend
    WIN32 MACOSX_BUNDLE
    main.cc
    main_window/main_window.cc
    main_window/main_window.h
    motor_control/motor_control.cc
    motor_control/motor_control.h
    scan/scan.cc
    scan/scan.h
    laser_control/laser_control.cc
    laser_control/laser_control.h
    emergency_stop/emergency_stop.cc
    emergency_stop/emergency_stop.h
)

target_compile_features(frontend PRIVATE cxx_std_17)
target_include_directories(frontend PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(frontend
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS frontend
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET frontend
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
