add_library(worker_interfaces STATIC worker_manager/worker_manager.cc worker_manager/worker_manager.h
    worker_types.h motor_worker/motor_worker.cc motor_worker/motor_worker.h
    laser_worker/laser_worker.cc laser_worker/laser_worker.h
    scan_worker/scan_worker.cc scan_worker/scan_worker.h)
target_include_directories(worker_interfaces PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(worker_interfaces PUBLIC
    Qt6::Core kinesis_devices ni_daq_laser algo_algorithm)
