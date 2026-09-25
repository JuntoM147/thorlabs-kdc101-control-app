#include <QApplication>

#include "main_window/main_window.h"
#include "application/application.h"
#include "main_window/application_binding.h"

int main(int argc, char* argv[]) {
  QApplication qt_application(argc, argv);

  application::Application backend;
  ui::MainWindow window;
  ui::BindApplication(window, backend);
  window.show();

  return qt_application.exec();
}
