# SDK locations can be overridden when configuring from Qt Creator or CMake.
set(KINESIS_ROOT "C:/Program Files/Thorlabs/Kinesis"
    CACHE PATH "Thorlabs Kinesis SDK directory")
find_path(KINESIS_INCLUDE_DIR Thorlabs.MotionControl.KCube.DCServo.h
    HINTS "${KINESIS_ROOT}" REQUIRED)
find_library(KINESIS_LIBRARY Thorlabs.MotionControl.KCube.DCServo
    HINTS "${KINESIS_ROOT}" REQUIRED)

set(NIDAQMX_ROOT "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C"
    CACHE PATH "NI-DAQmx C development support directory")
find_path(NIDAQMX_INCLUDE_DIR NIDAQmx.h
    HINTS "${NIDAQMX_ROOT}/include" REQUIRED)
find_library(NIDAQMX_LIBRARY NIDAQmx
    HINTS "${NIDAQMX_ROOT}/lib64/msvc" REQUIRED)

add_library(kinesis_devices STATIC
    kdc101/kdc101.cc
    kdc101/kdc101.h
    kdc101/kdc101_defaults.h
    kinesis_simulation/kinesis_simulation.h
)
target_include_directories(kinesis_devices PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}" "${KINESIS_INCLUDE_DIR}"
)
target_link_libraries(kinesis_devices PUBLIC project_error "${KINESIS_LIBRARY}")

add_library(ni_daq_laser STATIC
    ni_daq_laser/ni_daq_laser.cc
    ni_daq_laser/ni_daq_laser.h
)
target_include_directories(ni_daq_laser PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}" "${NIDAQMX_INCLUDE_DIR}"
)
target_link_libraries(ni_daq_laser PUBLIC project_error "${NIDAQMX_LIBRARY}")
