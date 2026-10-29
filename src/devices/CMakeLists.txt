set(NRT_X_TRAVEL_MM 150 CACHE STRING "X stage travel: 100 or 150 mm")
set(NRT_Y_TRAVEL_MM 100 CACHE STRING "Y stage travel: 100 or 150 mm")
set(NRT_Z_TRAVEL_MM 150 CACHE STRING "Z stage travel: 100 or 150 mm")
foreach(axis X Y Z)
    if(NOT NRT_${axis}_TRAVEL_MM MATCHES "^(100|150)$")
        message(FATAL_ERROR "NRT_${axis}_TRAVEL_MM must be 100 or 150.")
    endif()
endforeach()
set(PCIE6323_SERIAL_HEX "0185BDED" CACHE STRING "Expected PCIe-6323 serial, eight hexadecimal digits")
string(LENGTH "${PCIE6323_SERIAL_HEX}" serial_length)
if(NOT serial_length EQUAL 8 OR NOT PCIE6323_SERIAL_HEX MATCHES "^[0-9A-Fa-f]+$")
    message(FATAL_ERROR "PCIE6323_SERIAL_HEX must contain eight hexadecimal digits.")
endif()
set(PCIE6323_LASER_MODE "unconfigured" CACHE STRING "PCIe-6323 laser wiring: unconfigured or digital")
set_property(CACHE PCIE6323_LASER_MODE PROPERTY STRINGS unconfigured digital)
if(NOT PCIE6323_LASER_MODE MATCHES "^(unconfigured|digital)$")
    message(FATAL_ERROR "PCIE6323_LASER_MODE must be unconfigured or digital. Analog control is not implemented.")
endif()
set(PCIE6323_DIGITAL OFF)
if(PCIE6323_LASER_MODE STREQUAL "digital")
    set(PCIE6323_DIGITAL ON)
endif()

set(KINESIS_ROOT "C:/Program Files/Thorlabs/Kinesis" CACHE PATH "Thorlabs Kinesis SDK directory")
set(motor_sdk Thorlabs.MotionControl.Benchtop.StepperMotor)
find_path(KINESIS_NRT_INCLUDE_DIR "${motor_sdk}.h" HINTS "${KINESIS_ROOT}" REQUIRED)
find_library(KINESIS_NRT_LIBRARY "${motor_sdk}" HINTS "${KINESIS_ROOT}" REQUIRED)
add_library(kinesis_devices STATIC nrt100/nrt100.cc nrt100/nrt100.h
    kinesis_simulation/kinesis_simulation.h kinesis_simulation/kinesis_simulation.cc)
target_include_directories(kinesis_devices PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${KINESIS_NRT_INCLUDE_DIR}")
target_link_libraries(kinesis_devices PUBLIC project_error "${KINESIS_NRT_LIBRARY}")

set(NIDAQMX_ROOT "C:/Program Files (x86)/National Instruments/Shared/ExternalCompilerSupport/C"
    CACHE PATH "NI-DAQmx C development support directory")
find_path(NIDAQMX_INCLUDE_DIR NIDAQmx.h HINTS "${NIDAQMX_ROOT}/include" REQUIRED)
find_library(NIDAQMX_LIBRARY NIDAQmx HINTS "${NIDAQMX_ROOT}/lib64/msvc" REQUIRED)
add_library(pcie6323_laser STATIC pcie6323_laser/pcie6323_laser.cc pcie6323_laser/pcie6323_laser.h)
target_include_directories(pcie6323_laser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${NIDAQMX_INCLUDE_DIR}")
target_link_libraries(pcie6323_laser PUBLIC project_error "${NIDAQMX_LIBRARY}")

target_compile_definitions(kinesis_devices PUBLIC
    NRT_X_TRAVEL_MM=${NRT_X_TRAVEL_MM}
    NRT_Y_TRAVEL_MM=${NRT_Y_TRAVEL_MM}
    NRT_Z_TRAVEL_MM=${NRT_Z_TRAVEL_MM})
target_compile_definitions(pcie6323_laser PRIVATE
    PCIE6323_SERIAL=0x${PCIE6323_SERIAL_HEX}u
    PCIE6323_DIGITAL=$<BOOL:${PCIE6323_DIGITAL}>)
