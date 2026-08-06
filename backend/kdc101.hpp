#pragma once

extern "C" {
    __declspec(dllexport) int connect_device(void);

    __declspec(dllexport) int home_device(void);

    __declspec(dllexport) int move_position(const double position);

    __declspec(dllexport) double get_position(void);

    __declspec(dllexport) int move_relative(const double displacement);

    __declspec(dllexport) int jog(int direction);

    __declspec(dllexport) int close_device(void);

} // end extern "C"
