#include <QApplication>

#include "main_window/main_window.h"

int main(int argc, char* argv[]) {
  QApplication application(argc, argv);

  ui::MainWindow window;
  window.show();

  return application.exec();
}
