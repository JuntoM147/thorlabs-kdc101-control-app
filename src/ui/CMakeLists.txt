add_library(frontend_ui STATIC
    main_window/main_window.cc
    main_window/main_window.h
    main_window/worker_connections.cc
    ui_types.h
    shared/display_text.h
    motor_tab/motor_control/motor_control.cc
    motor_tab/motor_control/motor_control.h
    motor_tab/motor_information/motor_information.cc
    motor_tab/motor_information/motor_information.h
    scan_tab/image/image.cc
    scan_tab/image/image.h
    shared/status/status.cc
    shared/status/status.h
    scan_tab/scan_control/scan_control.cc
    scan_tab/scan_control/scan_control.h
    laser_tab/laser_control/laser_control.cc
    laser_tab/laser_control/laser_control.h
    options_tab/options/options.cc
    options_tab/options/options.h
)

target_include_directories(frontend_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(frontend_ui
    PUBLIC
        algo_algorithm
        worker_interfaces
        Qt6::Core
        Qt6::Widgets
)
