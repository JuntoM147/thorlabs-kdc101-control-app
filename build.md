cmake -S . -B build-app-cli -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/msvc2022_64" `
  -DBUILD_TESTING=OFF `
  -DAPPLICATION_USE_KINESIS_SIMULATION=OFF `
  -DDEVICES_USE_FAKE_KINESIS=OFF `
  -DDEVICES_USE_FAKE_NI_DAQ=OFF `
  -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$PWD/build-app-cli"

cmake --build build-app-cli --config Release --target frontend

.\build-app-cli\Release\frontend.exe
