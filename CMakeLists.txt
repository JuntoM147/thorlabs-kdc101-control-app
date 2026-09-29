cmake_minimum_required(VERSION 3.21)
project(confo_quanta LANGUAGES CXX)

if(MSVC)
    add_compile_options(/EHsc /utf-8)
endif()

include(CTest)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

add_subdirectory(src/error)
add_subdirectory(src/devices)
add_subdirectory(src/algo)
add_subdirectory(src/workers)
add_subdirectory(src/ui)

# Header-review stage: a UI preview, without a connected hardware backend.
qt_add_executable(frontend WIN32 MACOSX_BUNDLE src/main.cc)
target_link_libraries(frontend PRIVATE frontend_ui)
add_dependencies(frontend worker_interfaces)
if(WIN32)
    add_custom_command(TARGET frontend POST_BUILD
        COMMAND Qt6::windeployqt --no-translations --no-compiler-runtime "$<TARGET_FILE:frontend>"
        VERBATIM)
endif()
install(TARGETS frontend BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
qt_generate_deploy_app_script(TARGET frontend OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})

if(BUILD_TESTING)
    add_executable(refactor_test tests/refactor_test.cc)
    target_link_libraries(refactor_test PRIVATE frontend_ui)
    add_dependencies(refactor_test frontend)
    add_test(NAME refactor_test COMMAND refactor_test -platform offscreen)
    set_tests_properties(refactor_test PROPERTIES TIMEOUT 30)
    if(WIN32)
        add_custom_command(TARGET refactor_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:refactor_test>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>
                "$<TARGET_FILE_DIR:refactor_test>/platforms"
            VERBATIM)
    endif()
endif()
