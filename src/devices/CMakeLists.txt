cmake_minimum_required(VERSION 3.19)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Open the repository-root CMakeLists.txt to build the complete application.")
endif()

option(DEVICES_USE_FAKE_KINESIS "Build motor wrappers against the test SDK" OFF)
option(DEVICES_USE_FAKE_NI_DAQ "Build laser wrapper against the test SDK" OFF)

add_library(device_status STATIC
    device_status/device_status.cc
    device_status/device_status.h
)
target_compile_features(device_status PUBLIC cxx_std_23)
target_include_directories(device_status PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(kinesis_devices STATIC
    kdc101/kdc101.cc
    kdc101/kdc101.h
    kinesis_simulation/kinesis_simulation.cc
    kinesis_simulation/kinesis_simulation.h
)
target_compile_features(kinesis_devices PUBLIC cxx_std_23)
target_include_directories(kinesis_devices PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(kinesis_devices PUBLIC device_status)

if(DEVICES_USE_FAKE_KINESIS)
    target_include_directories(kinesis_devices PRIVATE ../../tests/kdc101/fake_sdk)
else()
    find_library(KINESIS_LIBRARY NAMES Thorlabs.MotionControl.KCube.DCServo
        HINTS "C:/Program Files/Thorlabs/Kinesis" REQUIRED)
    target_include_directories(kinesis_devices PRIVATE ../../external/thorlabs)
    target_link_libraries(kinesis_devices PUBLIC "${KINESIS_LIBRARY}")
endif()

if(DEVICES_USE_FAKE_NI_DAQ)
    set(NIDAQMX_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/ni_daq_laser/fake_sdk")
    set(NIDAQMX_LIBRARY "")
else()
    set(NIDAQMX_ROOT "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C"
        CACHE PATH "NI-DAQmx C development support directory")
    find_path(NIDAQMX_INCLUDE_DIR NIDAQmx.h HINTS "${NIDAQMX_ROOT}/include" REQUIRED)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_ni_library_dir "${NIDAQMX_ROOT}/lib64/msvc")
    else()
        set(_ni_library_dir "${NIDAQMX_ROOT}/lib32/msvc")
    endif()
    find_library(NIDAQMX_LIBRARY NIDAQmx HINTS "${_ni_library_dir}" REQUIRED)
endif()

add_library(ni_daq_laser STATIC
    ni_daq_laser/ni_daq_laser.cc
    ni_daq_laser/ni_daq_laser.h
)
target_compile_features(ni_daq_laser PUBLIC cxx_std_23)
target_include_directories(ni_daq_laser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${NIDAQMX_INCLUDE_DIR}")
target_link_libraries(ni_daq_laser PUBLIC device_status "${NIDAQMX_LIBRARY}")

if(BUILD_TESTING AND DEVICES_USE_FAKE_KINESIS)
    add_executable(kdc101_nonblocking_test ../../tests/kdc101/nonblocking_test.cc)
    target_include_directories(kdc101_nonblocking_test PRIVATE ../../tests/kdc101/fake_sdk)
    target_link_libraries(kdc101_nonblocking_test PRIVATE kinesis_devices)
    add_test(NAME kdc101_nonblocking_test COMMAND kdc101_nonblocking_test)
endif()
