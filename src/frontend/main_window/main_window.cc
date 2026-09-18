#include "main_window/main_window.h"

#include <QHBoxLayout>
#include <QStatusBar>
#include <QWidget>
#include <QVBoxLayout>

#include "motor_information/motor_information.h"
#include "motor_control/motor_control.h"
#include "scan/scan.h"
#include "scan_control/scan_control.h"

namespace ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(tr("Confo Quanta"));
  resize(1400, 900);

  setStyleSheet(QStringLiteral(R"(
    QMainWindow, QWidget {
      background: #f7f9fc;
      color: #14233f;
      font-family: "Segoe UI";
      font-size: 13px;
    }
    QGroupBox {
      background: #ffffff;
      border: 1px solid #dce3ed;
      border-radius: 8px;
      margin-top: 14px;
      padding: 14px 10px 10px;
      font-size: 16px;
      font-weight: 600;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      left: 14px;
      padding: 0 6px;
      color: #14233f;
    }
    QLabel { background: transparent; }
    QPushButton {
      background: #ffffff;
      border: 1px solid #cfd8e5;
      border-radius: 5px;
      padding: 7px 14px;
      min-height: 20px;
    }
    QPushButton:hover { background: #edf4ff; border-color: #80b3ff; }
    QPushButton:pressed { background: #dceaff; }
    QPushButton:focus { border: 2px solid #126bf0; padding: 6px 13px; }
    QPushButton:disabled {
      background: #edf0f4;
      color: #8794a8;
      border-color: #e0e5ec;
    }
    QPushButton[primary="true"] {
      background: #126bf0;
      color: white;
      border-color: #126bf0;
      font-weight: 600;
    }
    QPushButton[primary="true"]:hover { background: #095bd4; }
    QPushButton[primary="true"]:pressed { background: #084bad; }
    QPushButton[primary="true"]:disabled {
      background: #edf0f4;
      color: #8794a8;
      border-color: #e0e5ec;
    }
    QLineEdit, QDoubleSpinBox {
      background: #ffffff;
      border: 1px solid #cfd8e5;
      border-radius: 4px;
      padding: 5px;
      min-height: 20px;
    }
    QLineEdit:focus, QDoubleSpinBox:focus { border-color: #126bf0; }
    QLineEdit:read-only { background: #f0f4f9; color: #52627c; }
    QDoubleSpinBox:disabled { background: #f3f5f8; color: #8794a8; }
    QScrollArea {
      background: #f9fbfe;
      border: 1px solid #dce3ed;
      border-radius: 4px;
    }
    QProgressBar {
      background: #e5eaf1;
      border: 1px solid #d5dde8;
      border-radius: 4px;
      min-height: 22px;
      text-align: center;
    }
    QProgressBar::chunk { background: #126bf0; border-radius: 3px; }
    QStatusBar { background: #f7f9fc; color: #52627c; }
    QGroupBox#axisZ { border-left: 5px solid #126bf0; }
    QGroupBox#axisX { border-left: 5px solid #00a34a; }
    QGroupBox#axisY { border-left: 5px solid #ff7b13; }
    QPushButton#laserStatus {
      background: #fff0f1;
      color: #d9233c;
      border: 1px solid #ffb8c1;
      border-radius: 6px;
      font-size: 20px;
      font-weight: 600;
      padding: 12px;
    }
    QPushButton#laserStatus:checked {
      background: #e9f8ef;
      color: #00843b;
      border-color: #8bd5ac;
    }
    QPushButton#laserStatus:hover { background: #ffe3e7; }
    QPushButton#laserStatus:checked:hover { background: #d6f1e1; }
    QPushButton#laserStatus:focus { border: 2px solid #126bf0; padding: 11px; }
  )"));

  auto* central_widget = new QWidget(this);

  auto* layout = new QHBoxLayout(central_widget);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(16);

  auto* left_column = new QWidget(central_widget);
  auto* left_layout = new QVBoxLayout(left_column);
  left_layout->setContentsMargins(0, 0, 0, 0);
  left_layout->setSpacing(12);
  left_layout->addWidget(CreateMotorControlSection(left_column), 1);
  left_layout->addWidget(CreateMotorInformationSection(left_column));
  layout->addWidget(left_column, 2);

  auto* right_column = new QWidget(central_widget);
  auto* right_layout = new QVBoxLayout(right_column);
  right_layout->setContentsMargins(0, 0, 0, 0);
  right_layout->setSpacing(12);
  right_layout->addWidget(CreateScanSection(right_column), 1);
  right_layout->addWidget(CreateScanControlSection(right_column));
  layout->addWidget(right_column, 3);

  setCentralWidget(central_widget);
  statusBar()->showMessage(tr("Hardware not connected."));
}

}  // namespace ui
