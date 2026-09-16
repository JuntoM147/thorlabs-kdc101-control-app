#include "main_window/main_window.h"

#include <QHBoxLayout>
#include <QStatusBar>
#include <QWidget>
#include <QVBoxLayout>

#include "emergency_stop/emergency_stop.h"
#include "laser_control/laser_control.h"
#include "motor_control/motor_control.h"
#include "scan/scan.h"

namespace ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(tr("Confo Quanta"));
  resize(1400, 900);

  auto* central_widget = new QWidget(this);

  auto* layout = new QHBoxLayout(central_widget);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(16);

  // motor control
  layout->addWidget(CreateMotorControlSection(central_widget), 2);
  
  // scan
  layout->addWidget(CreateScanSection(central_widget), 2);

  // laser control and emergency stop
  auto* right_column = new QWidget(central_widget);
  auto* right_layout = new QVBoxLayout(right_column);
  right_layout->setContentsMargins(0, 0, 0, 0);
  right_layout->setSpacing(16);
  right_layout->addWidget(CreateLaserControlSection(right_column));
  right_layout->addWidget(CreateEmergencyStopSection(right_column));
  right_layout->addStretch();
  layout->addWidget(right_column, 1);

  setCentralWidget(central_widget);
  statusBar()->showMessage(tr("Hardware not connected."));
}

}  // namespace ui
