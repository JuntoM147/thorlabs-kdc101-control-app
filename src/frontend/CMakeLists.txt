cmake_minimum_required(VERSION 3.19)

# Support configuring this directory independently of the repository root.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(confo_quanta_frontend LANGUAGES CXX)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

add_library(frontend_ui STATIC
    main_window/main_window.cc
    main_window/main_window.h
    main_window/application_binding.h
    main_window/display_text.h
    motor_control/motor_control.cc
    motor_control/motor_control.h
    motor_information/motor_information.cc
    motor_information/motor_information.h
    scan/scan.cc
    scan/scan.h
    scan_control/scan_control.cc
    scan_control/scan_control.h
    laser_control/laser_control.cc
    laser_control/laser_control.h
)

if(NOT TARGET algo_conversion)
    add_subdirectory(../algo "${CMAKE_CURRENT_BINARY_DIR}/algo")
endif()

target_compile_features(frontend_ui PUBLIC cxx_std_23)
target_include_directories(frontend_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(frontend_ui
    PUBLIC
        algo_conversion
        Qt::Core
        Qt::Widgets
)

qt_add_executable(frontend WIN32 MACOSX_BUNDLE main.cc)
target_link_libraries(frontend PRIVATE frontend_ui)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(application_binding_test ../../tests/frontend/application_binding_test.cc)
    target_link_libraries(application_binding_test PRIVATE frontend_ui Qt6::Test)
    add_test(NAME application_binding_test COMMAND application_binding_test -platform offscreen)
    set_tests_properties(application_binding_test PROPERTIES TIMEOUT 30)
    # Compile real declarations; the fake SDK supplies header types only.
    add_library(application_binding_contract OBJECT ../../tests/frontend/application_binding_contract.cc)
    target_link_libraries(application_binding_contract PRIVATE frontend_ui)
    target_include_directories(application_binding_contract PRIVATE
        ../devices ../../tests/ni_daq_laser/fake_sdk)
    if(WIN32)
        add_custom_command(TARGET application_binding_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Core> $<TARGET_FILE:Qt6::Gui>
                $<TARGET_FILE:Qt6::Widgets> $<TARGET_FILE:Qt6::Test>
                $<TARGET_FILE_DIR:application_binding_test>
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:application_binding_test>/plugins/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>
                "$<TARGET_FILE_DIR:application_binding_test>/plugins/platforms"
            VERBATIM)
    endif()
endif()

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
