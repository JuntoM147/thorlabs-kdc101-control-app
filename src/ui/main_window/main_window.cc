#include "main_window/main_window.h"

#include <QHBoxLayout>
#include <QStringList>
#include <QWidget>
#include <QVBoxLayout>
#include <utility>

#include "motor_information/motor_information.h"
#include "motor_control/motor_control.h"
#include "image/image.h"
#include "laser_control/laser_control.h"
#include "status/status.h"
#include "scan_control/scan_control.h"

namespace ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(tr("Confo Quanta"));
  resize(1400, 900);
  // Temporary scan defaults until calibration/exposure controls are introduced.
  scan_configuration_.pixel_size_mm = 0.0001;  // 0.1 micrometres per pixel.
  scan_configuration_.exposure_time = std::chrono::milliseconds(10);

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
    QLineEdit, QDoubleSpinBox, QSpinBox {
      background: #ffffff;
      border: 1px solid #cfd8e5;
      border-radius: 4px;
      padding: 5px;
      min-height: 20px;
    }
    QLineEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus { border-color: #126bf0; }
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
    QGroupBox#motorControls, QGroupBox#axisX, QGroupBox#axisY, QGroupBox#axisZ {
      padding: 4px;
      margin-top: 12px;
    }
    QLabel#axisFieldLabel { color: #52627c; font-size: 12px; }
    QGroupBox#motorControls QPushButton { padding: 3px 8px; }
    QGroupBox#motorControls QPushButton:focus { padding: 2px 7px; }
    QGroupBox#motorControls QLineEdit, QGroupBox#motorControls QDoubleSpinBox {
      padding: 2px 5px;
    }
    QGroupBox#motorControls QLineEdit:disabled, QGroupBox#motorControls QComboBox:disabled {
      background: #f3f5f8;
      color: #8794a8;
    }
    QGroupBox#axisX, QGroupBox#axisY, QGroupBox#axisZ {
      font-size: 13px;
    }
    QGroupBox#motorControls QPushButton#axisStop {
      color: #b42338;
      border-color: #d9919b;
      background: #ffffff;
    }
    QGroupBox#motorControls QPushButton#axisStop:hover { background: #fff0f1; }
    QGroupBox#motorControls QPushButton#axisStop:pressed { background: #ffe0e4; }
    QGroupBox#motorControls QPushButton#axisStop:disabled {
      background: #edf0f4;
      color: #8794a8;
      border-color: #e0e5ec;
    }
    QGroupBox#axisZ { border-left: 3px solid #126bf0; }
    QGroupBox#axisX { border-left: 3px solid #00a34a; }
    QGroupBox#axisY { border-left: 3px solid #ff7b13; }
  )"));

  auto* central_widget = new QWidget(this);

  auto* layout = new QHBoxLayout(central_widget);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(16);

  auto* left_column = new QWidget(central_widget);
  auto* left_layout = new QVBoxLayout(left_column);
  left_layout->setContentsMargins(0, 0, 0, 0);
  left_layout->setSpacing(12);
  left_layout->addWidget(CreateMotorControlSection(*this, left_column), 1);
  left_layout->addWidget(CreateMotorInformationSection(*this, left_column));
  layout->addWidget(left_column, 2);

  auto* right_column = new QWidget(central_widget);
  auto* right_layout = new QVBoxLayout(right_column);
  right_layout->setContentsMargins(0, 0, 0, 0);
  right_layout->setSpacing(12);
  auto* upper = new QHBoxLayout();
  upper->setSpacing(12);
  upper->addWidget(CreateImageSection(*this, right_column), 3);
  auto* side = new QVBoxLayout();
  side->setSpacing(12);
  side->addWidget(CreateStatusSection(*this, right_column));
  side->addWidget(CreateLaserControlSection(*this, right_column), 1);
  upper->addLayout(side, 2);
  right_layout->addLayout(upper, 1);
  right_layout->addWidget(CreateScanControlSection(*this, right_column));
  layout->addWidget(right_column, 3);

  connect(this, &MainWindow::AxisStateUpdated, this, [this](application::AxisState state) {
    const auto index = static_cast<std::size_t>(state.axis);
    if (index >= motor_connections_.size() || motor_connections_[index] == state.connection) return;
    motor_connections_[index] = state.connection;
    emit ScanAvailabilityChanged();
  });
  connect(this, &MainWindow::LaserStateUpdated, this, [this](application::LaserState state) {
    if (laser_connection_ == state.connection) return;
    laser_connection_ = state.connection;
    emit ScanAvailabilityChanged();
  });
  setCentralWidget(central_widget);
  SetBackendAvailable(false);
}

void MainWindow::SetBackendAvailable(bool available) {
  const bool lost_backend = backend_available_ && !available;
  backend_available_ = available;
  if (!available) {
    motor_connections_.fill(application::ConnectionState::kDisconnected);
    laser_connection_ = application::ConnectionState::kDisconnected;
  }
  emit ManualControlsEnabled(available && !controls_locked_);
  emit ScanInputsEnabled(!controls_locked_);
  emit ScanAvailabilityChanged();
  if (lost_backend) ShowMessage(tr("Application backend is not available."), true);
}

void MainWindow::SetControlsLocked(bool locked) {
  controls_locked_ = locked;
  emit ManualControlsEnabled(backend_available_ && !locked);
  emit ScanInputsEnabled(!locked);
  emit ScanAvailabilityChanged();
}

void MainWindow::SetScanImage(const QImage& image) {
  scan_image_size_ = image.size();
  start_pixel_set_ = false;
  has_pattern_ = false;
  scan_configuration_.pattern = {};
  if (!image.isNull()) {
    auto result = algo::Convert(image);
    if (result && result->PixelCount() > 0) {
      scan_configuration_.pattern = std::move(*result);
      has_pattern_ = true;
      ShowMessage(tr("Image imported. Set a start pixel before starting the scan."));
    } else {
      ShowError({{}, "Import image", result ? "The image has no exposed pixels." : result.error()});
    }
  }
  emit ScanAvailabilityChanged();
}

void MainWindow::SetStartPixel(int x, int y) {
  if (!CanSetStartPixel()) return;
  if (x < 0 || y < 0 || x >= scan_image_size_.width() || y >= scan_image_size_.height()) {
    ShowMessage(tr("Set start: Enter a nonnegative pixel inside the image."), true);
    return;
  }
  scan_configuration_.start_pixel = {x, y};
  start_pixel_set_ = true;
  ShowMessage(tr("Start pixel set to (%1, %2).").arg(x).arg(y));
  emit ScanAvailabilityChanged();
}

QStringList MainWindow::ScanStartBlockers() const {
  QStringList reasons;
  if (!backend_available_) reasons << tr("Application backend is unavailable.");
  if (controls_locked_ || scan_state_.phase != application::ScanPhase::kIdle)
    reasons << tr("Wait until the scan is idle and controls are unlocked.");
  const QStringList axes{"X", "Y", "Z"};
  for (std::size_t i = 0; i < motor_connections_.size(); ++i) {
    if (motor_connections_[i] != application::ConnectionState::kConnected)
      reasons << tr("Connect the %1 motor.").arg(axes.at(i));
  }
  if (laser_connection_ != application::ConnectionState::kConnected)
    reasons << tr("Connect the laser.");
  if (!has_pattern_) reasons << tr("Import an image with exposed pixels.");
  if (!start_pixel_set_ || scan_configuration_.start_pixel.x >= scan_image_size_.width() ||
      scan_configuration_.start_pixel.y >= scan_image_size_.height())
    reasons << tr("Apply a valid starting pixel using Set start.");
  return reasons;
}

bool MainWindow::CanStartScan() const {
  return ScanStartBlockers().isEmpty();
}

void MainWindow::RequestScan() {
  if (!CanStartScan()) return;
  emit StartScanRequested(scan_configuration_);
}

void MainWindow::UpdateScanState(application::ScanState state) {
  scan_state_ = state;
  emit ScanDisplayChanged(state);
  emit ScanAvailabilityChanged();
}

void MainWindow::ShowMessage(const QString& message, bool error) {
  emit StatusMessageChanged(message, error);
}

void MainWindow::ShowError(application::OperationError error) {
  const QString axis = error.axis
      ? tr("Axis %1: ").arg(QStringList{"X", "Y", "Z"}.at(static_cast<int>(*error.axis)))
      : QString();
  ShowMessage(axis + QString::fromStdString(error.operation) +
              ": " + QString::fromStdString(error.message), true);
}
}  // namespace ui
