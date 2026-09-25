cmake_minimum_required(VERSION 3.19)

project(confo_quanta LANGUAGES CXX)

include(CTest)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()
qt_standard_project_setup()

add_subdirectory(src/algo)
add_subdirectory(src/devices)
add_subdirectory(src/application)
add_subdirectory(src/frontend)
