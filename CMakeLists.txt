cmake_minimum_required(VERSION 3.21)
project(confo_quanta LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/EHsc /utf-8 /Zc:__cplusplus)
endif()
if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()
# The UI is built in C++; there are no Designer .ui files to generate.
set(CMAKE_AUTOUIC OFF)

add_subdirectory(src/error)
add_subdirectory(src/devices)
add_subdirectory(src/algo)
add_subdirectory(src/workers)
add_subdirectory(src/ui)

# One executable. The subdirectories define its supporting libraries only.
qt_add_executable(frontend WIN32 src/main.cc)
target_link_libraries(frontend PRIVATE frontend_ui worker_interfaces)

# Make the build output runnable directly from Qt Creator or Explorer.
if(WIN32)
    add_custom_command(TARGET frontend POST_BUILD
        COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime
            "$<TARGET_FILE:frontend>"
        VERBATIM
    )
endif()
