#include "main_window/main_window.h"

#include <QHBoxLayout>
#include <QStatusBar>
#include <QWidget>

#include "laser_control/laser_control.h"
#include "motor_control/motor_control.h"
#include "scan/scan.h"

namespace ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(tr("Confo Quanta"));
  resize(1100, 600);
  auto* central_widget = new QWidget(this);
  auto* layout = new QHBoxLayout(central_widget);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(16);
  layout->addWidget(CreateMotorControlSection(central_widget), 1);
  layout->addWidget(CreateScanSection(central_widget), 2);
  layout->addWidget(CreateLaserControlSection(central_widget), 1);
  setCentralWidget(central_widget);
  statusBar()->showMessage(tr("Hardware not connected."));
}

}  // namespace ui
