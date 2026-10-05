add_library(project_error STATIC error.cc error.h)
target_include_directories(project_error PUBLIC "${PROJECT_SOURCE_DIR}/src")
