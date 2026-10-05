#include <QApplication>

#include "main_window/main_window.h"
#include "workers/worker_manager/worker_manager.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  workers::WorkerManager workers;
  ui::MainWindow window;
  window.ConnectWorkers(workers);
  QObject::connect(&app, &QCoreApplication::aboutToQuit, &window, [&] {
    window.SetWorkersAvailable(false);
    workers.Shutdown();
  });
  window.show();
  return app.exec();
}
