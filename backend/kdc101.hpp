#pragma once

enum class JogDirection { // Note to self: had to use enum class to avoid name collision
    Forwards = 1,
    Backwards = -1
};


extern "C" {
    __declspec(dllexport) int find_device(void);

    __declspec(dllexport) int initialize(void);

    __declspec(dllexport) void initialize_simulation(void);

    __declspec(dllexport) void uninitialize_simulation(void);

    __declspec(dllexport) int home_device(void);

    __declspec(dllexport) int move_position(const double position);

    __declspec(dllexport) double get_position(void);

    __declspec(dllexport) int move_relative(const double displacement);

    __declspec(dllexport) int jog(JogDirection direction);

    __declspec(dllexport) int close(void);

} // end extern "C"
