cmake -S . -B build-app-vs2026 -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64" -DBUILD_TESTING=OFF -DAPPLICATION_USE_KINESIS_SIMULATION=OFF -DDEVICES_USE_FAKE_KINESIS=OFF -DDEVICES_USE_FAKE_NI_DAQ=OFF -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$PWD/build-app-vs2026"

cmake --build build-app-cli --config Release --target frontend

.\build-app-cli\Release\frontend.exe

& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -all -products "*" -property displayName

cmake --version