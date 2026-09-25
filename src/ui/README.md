# Application wiring

`MainWindow` emits typed user requests. `BindApplication(window, backend)` connects
them to `Application` and connects its observations, scan state, lock, and errors
back to the display. Call it once, on the GUI thread, before showing the window.
The template permits a recording backend in UI tests; it adds no runtime layer.

The repository-root build links the application library and binds the real backend
in `main.cc`. Only the repository-root build is supported; there is no UI-only
executable. The executable always constructs objects in this order:

```cpp
QApplication qt_application(argc, argv);
application::Application backend;
ui::MainWindow window;
ui::BindApplication(window, backend);
window.show();
return qt_application.exec();
```

Include `application/application.h` from the application include directory and
`main_window/application_binding.h`. The window is destroyed before the backend.

Image conversion uses `algo::Convert` because `StartScan` accepts a `BinaryMatrix`.
Starting pixel X/Y are zero-based image coordinates, applied with Set start.
Start remains disabled until a valid start pixel is applied for the current image;
importing or clearing an image resets this requirement.
The applied pixel maps to the current stage position; draft edits do not affect scans.
The UI uses fixed defaults of 0.1 micrometres per pixel and 10 ms exposure.
Motion settings and timeout retain the application defaults. Manual controls are
disabled during scan ownership, including while paused. The application must
still reject disallowed requests independently of these display restrictions.

`application_binding_test` tests requests and updates without hardware.
`application_binding_contract` compiles the same binding against the real headers.
Neither test claims to exercise device execution or thread lifetime.
