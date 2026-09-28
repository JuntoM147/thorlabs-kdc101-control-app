cmake_minimum_required(VERSION 3.19)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Open the repository-root CMakeLists.txt to build the complete application.")
endif()

add_library(frontend_ui STATIC
    main_window/main_window.cc
    main_window/main_window.h
    main_window/application_binding.h
    shared/display_text.h
    motor_tab/motor_control/motor_control.cc
    motor_tab/motor_control/motor_control.h
    motor_tab/motor_information/motor_information.cc
    motor_tab/motor_information/motor_information.h
    scan_tab/image/image.cc
    scan_tab/image/image.h
    scan_tab/route_preview/route_preview.cc
    scan_tab/route_preview/route_preview.h
    shared/status/status.cc
    shared/status/status.h
    scan_tab/scan_control/scan_control.cc
    scan_tab/scan_control/scan_control.h
    laser_tab/laser_control/laser_control.cc
    laser_tab/laser_control/laser_control.h
    options_tab/options/options.cc
    options_tab/options/options.h
)

target_compile_features(frontend_ui PUBLIC cxx_std_23)
target_include_directories(frontend_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(frontend_ui
    PUBLIC
        algo_algorithm
        Qt::Core
        Qt::Widgets
)

qt_add_executable(frontend WIN32 MACOSX_BUNDLE main.cc)
target_link_libraries(frontend PRIVATE frontend_ui application)
if(WIN32)
    # Qt Creator runs from the build tree; install-time deployment is too late.
    # Include the native Windows platform plugin, not just the test offscreen plugin.
    add_custom_command(TARGET frontend POST_BUILD
        COMMAND Qt6::windeployqt
            --no-translations --no-compiler-runtime
            "$<TARGET_FILE:frontend>"
        VERBATIM)
endif()
if(BUILD_TESTING)
    qt_add_executable(application_binding_test ../../tests/frontend/application_binding_test.cc)
    target_link_libraries(application_binding_test PRIVATE frontend_ui Qt6::Test)
    add_test(NAME application_binding_test COMMAND application_binding_test -platform offscreen)
    set_tests_properties(application_binding_test PROPERTIES TIMEOUT 30)
    if(DEVICES_USE_FAKE_KINESIS AND DEVICES_USE_FAKE_NI_DAQ)
        qt_add_executable(application_runtime_test ../../tests/frontend/application_runtime_test.cc)
        target_link_libraries(application_runtime_test PRIVATE frontend_ui application Qt6::Test)
        target_include_directories(application_runtime_test PRIVATE ../../tests/kdc101/fake_sdk)
        add_test(NAME application_runtime_test COMMAND application_runtime_test -platform offscreen)
        set_tests_properties(application_runtime_test PROPERTIES TIMEOUT 60)
    endif()
    # Compile real declarations; the fake SDK supplies header types only.
    add_library(application_binding_contract OBJECT ../../tests/frontend/application_binding_contract.cc)
    target_link_libraries(application_binding_contract PRIVATE frontend_ui)
    target_include_directories(application_binding_contract PRIVATE
        ../devices ../../tests/ni_daq_laser/fake_sdk)
    if(WIN32)
      foreach(test_target IN ITEMS application_binding_test application_runtime_test)
        if(NOT TARGET ${test_target})
            continue()
        endif()
        add_custom_command(TARGET ${test_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Core> $<TARGET_FILE:Qt6::Gui>
                $<TARGET_FILE:Qt6::Widgets> $<TARGET_FILE:Qt6::Test>
                $<TARGET_FILE_DIR:${test_target}>
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${test_target}>/plugins/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>
                "$<TARGET_FILE_DIR:${test_target}>/plugins/platforms"
            VERBATIM)
      endforeach()
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
