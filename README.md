# About

Qt UI for KDC101 motors, an NI-DAQ laser output, and image-based scan planning.

# Dependencies

The dependencies to compile and run this application are listed in the table below:

| Dependency | Purpose | Download |
| --- | --- | --- |
| Qt 6.5+ (Core and Widgets, MSVC kit) | UI | [Qt](https://www.qt.io) |
| CMake 3.21+ | Build configuration | [CMake](https://cmake.org/download/) |
| MSVC with C++23 | Compiler | [Visual Studio / Build Tools](https://visualstudio.microsoft.com/downloads/) |
| Thorlabs Kinesis SDK | KDC101 motor control | [Kinesis](https://www.thorlabs.com/software-pages/motion_control/) |
| NI-DAQmx library | Laser control | [NI-DAQmx](https://www.ni.com/en/support/downloads/drivers/download.ni-daq-mx.html) |

# Build

```powershell
cmake --preset app -DCMAKE_PREFIX_PATH=C:/Qt/6.11.2/msvc2022_64
cmake --build --preset app
```


# Coding Style

I have tried as much as possible to follow Google's C++ Coding Standard

See: https://google.github.io/styleguide/cppguide.html#C++_Version

# Layout Style

For project organisation I have tried to follow The Pitchfork Layout (PFL) convention

See: https://joholl.github.io/pitchfork-website/

# AI Usage

ChatGPT codex was used in this project to help identify bugs, suggest code fixes, and to discuss the underlying algorithm and structure of the project.
