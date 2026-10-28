# Project

Windows C++23 application using Qt6, CMake, MSVC, Thorlabs Kinesis, and NI-DAQmx. 

See `README.md` for dependencies, hardware profiles, and build commands.

# Style

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html). The project's C++23 requirement and rules below take precedence; `.clang-format` controls formatting.
- Keep code minimal and readable. Prefer explicit types unless `auto` significantly improves readability. Preserve names required by Qt and other external interfaces.
- Minimize header dependencies without adding PImpl or extra layers solely to hide private members.
- After editing C++, run `clang-format -i -style=file` with explicit paths to the files changed for the task, including new files. Sources use nested `.cc` and `.h` paths under `src/`; do not format unrelated files. Documentation-only changes need no formatting command.

# Workflow
- Inspect the working tree and preserve existing user changes. Keep cleanup within the task's scope.
- Ask before changing `.clang-format` or introducing architectural patterns that add files or layers, unless already authorized.
