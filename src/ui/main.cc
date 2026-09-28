#include <QApplication>

#include "main_window/main_window.h"
#include "application/application.h"
#include "main_window/application_binding.h"

int main(int argc, char* argv[]) {
  QApplication qt_application(argc, argv);

  application::Application backend;
  ui::MainWindow window;
  ui::BindApplication(window, backend);
  if (backend.UsesSimulatedMotors()) {
    window.setWindowTitle(window.windowTitle() + QObject::tr(" — SIMULATED MOTORS"));
    window.ShowMessage(QObject::tr("Motor simulation is enabled. Physical motors will not move; the laser uses NI-DAQ hardware."));
  }
  window.show();

  return qt_application.exec();
}
