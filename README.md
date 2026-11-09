# About

This project is a **Windows** desktop application for **Confo Quanta's laser writing product**. 
Operators can control the position of a stage by controlling the motors through the GUI and load an image for the device to draw. 

The application is written in **C++** with a user interface made using the **Qt** library. 

# Getting started

Install [Git for Windows](https://gitforwindows.org/) if needed. 

Open **Command Prompt** in the folder where you want to keep the project and run:

```bat
git clone https://github.com/JuntoM147/thorlabs-kdc101-control-app.git
cd thorlabs-kdc101-control-app
```

On 64-bit Windows, you will need:

- [Visual Studio 2022 or Build Tools 2022](https://visualstudio.microsoft.com/vs/older-downloads/) with **Desktop development with C++** (MSVC v143 and Windows SDK).
- [CMake 3.21 or newer](https://cmake.org/download/) on your PATH.
- [Qt 6.5 or newer](https://www.qt.io/download-qt-installer-oss) with the MSVC 64-bit desktop kit (Core, Gui, and Widgets).
- [Thorlabs Kinesis](https://www.thorlabs.com/software-pages/motion_control/) with the 64-bit benchtop stepper SDK and runtime.
- [NI-DAQmx](https://www.ni.com/en/support/downloads/drivers/download.ni-daq-mx.html) with the driver and C development support.

In File Explorer, open your Qt installation folder, then your Qt version, and copy the full path of the 64-bit MSVC kit folder containing `bin`, `include`, and `lib` (for example, `C:\Qt\6.11.2\msvc2022_64`) for both commands below.

To check you have installed all the dependencies, run this from the repository root in **Command Prompt**, using your Qt kit path:

```bat
powershell -NoProfile -ExecutionPolicy RemoteSigned -File tools\check_dependencies.ps1 -QtDir "C:/Qt/6.11.2/msvc2022_64"
```

To build, run this line in **Command Prompt** from the repository root with the same Qt kit path. CMake checks the compiler and development libraries before building the Release application.

```bat
cmake --preset app -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64" && cmake --build --preset app
```

Launch `build-app\Release\frontend.exe` to check that the runtime libraries load.

# Documentation

For architecture and design justifications please refer to the [documentation here](docs/README.md)

# Coding Style

I have tried as much as possible to follow Google's C++ Coding Standard

See: https://google.github.io/styleguide/cppguide.html#C++_Version

# Layout Style

For project organisation I have tried to follow The Pitchfork Layout (PFL) convention

See: https://joholl.github.io/pitchfork-website/

# AI Usage

ChatGPT codex was used in this project to help identify bugs, suggest code fixes, and to discuss the underlying algorithm and structure of the project.
