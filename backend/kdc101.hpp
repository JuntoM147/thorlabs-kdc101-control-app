#pragma once

extern "C" {
    __declspec(dllexport) void initialize_simulation(void);

    __declspec(dllexport) void uninitialize_simulation(void);

    __declspec(dllexport) int connect_device(const char* serialNo);

    __declspec(dllexport) int home_device(const char* serialNo);

    __declspec(dllexport) int move_position(const char* serialNo, double position);

    __declspec(dllexport) double get_position(const char* serialNo);

    __declspec(dllexport) int jog(const char* serialNo, int direction);

    __declspec(dllexport) int start_drive(const char* serialNo, int direction);

    __declspec(dllexport) int stop_drive(const char* serialNo);

    __declspec(dllexport) int close_device(const char* serialNo);

} // end extern "C"
