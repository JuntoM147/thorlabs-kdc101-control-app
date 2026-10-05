#include "main_window/main_window.h"

#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>
#include <utility>

#include "laser_tab/laser_control/laser_control.h"
#include "motor_tab/motor_control/motor_control.h"
#include "motor_tab/motor_information/motor_information.h"
#include "options_tab/options/options.h"
#include "scan_tab/image/image.h"
#include "scan_tab/scan_control/scan_control.h"
#include "shared/status/status.h"

namespace ui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(tr("Confo Quanta"));
  setFixedSize(680, 780);
  // Initial values, with pixel size editable in the Laser tab.
  scan_configuration_.pixel_size_mm = 0.00025;  // 0.25 micrometres per pixel.
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
      margin-top: 12px;
      padding: 6px 8px 4px;
      font-size: 15px;
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
      padding: 3px 10px;
      min-height: 18px;
    }
    QPushButton:hover { background: #edf4ff; border-color: #80b3ff; }
    QPushButton:pressed { background: #dceaff; }
    QPushButton:focus { border: 2px solid #126bf0; padding: 2px 9px; }
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
      padding: 3px 5px;
      min-height: 18px;
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
      padding: 2px 4px;
      margin-top: 12px;
    }
    QLabel#axisFieldLabel { color: #52627c; font-size: 13px; }
    QGroupBox#motorControls QPushButton { padding: 2px 8px; }
    QGroupBox#motorControls QPushButton:focus { padding: 1px 7px; }
    QGroupBox#motorControls QLineEdit, QGroupBox#motorControls QDoubleSpinBox {
      padding: 1px 5px;
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

  auto* tabs = new QTabWidget(this);
  const auto add_page = [this, tabs](const QString& title) {
    auto* page = new QWidget(tabs);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(6);
    layout->addWidget(CreateStatusSection(*this, page));
    tabs->addTab(page, title);
    return layout;
  };

  auto* motor_layout = add_page(tr("Motor"));
  auto* motor_page = motor_layout->parentWidget();
  motor_layout->addWidget(CreateMotorControlSection(*this, motor_page), 1);
  motor_layout->addWidget(CreateMotorInformationSection(*this, motor_page));

  auto* scan_layout = add_page(tr("Scan"));
  auto* scan_page = scan_layout->parentWidget();
  scan_layout->addWidget(CreateImageSection(*this, scan_page), 1);
  scan_layout->addWidget(CreateScanControlSection(*this, scan_page));

  auto* laser_layout = add_page(tr("Laser"));
  laser_layout->addWidget(
      CreateLaserControlSection(*this, laser_layout->parentWidget()));
  laser_layout->addStretch();

  auto* options_layout = add_page(tr("Options"));
  options_layout->addWidget(
      CreateOptionsSection(*this, options_layout->parentWidget()), 1);
  ConnectStatusMessages(*this);

  connect(this, &MainWindow::AxisStateUpdated, this,
          [this](ui::AxisState state) {
            const auto index = static_cast<std::size_t>(state.axis);
            if (index >= motor_connections_.size()) return;
            if (motor_connections_[index] == state.connection &&
                motor_operations_[index] == state.operation &&
                motor_homed_[index] == state.homed)
              return;
            motor_connections_[index] = state.connection;
            motor_operations_[index] = state.operation;
            motor_homed_[index] = state.homed;
            emit ScanAvailabilityChanged();
          });
  connect(this, &MainWindow::LaserStateUpdated, this,
          [this](ui::LaserState state) {
            if (laser_connection_ == state.connection &&
                laser_output_ == state.output_enabled)
              return;
            laser_connection_ = state.connection;
            laser_output_ = state.output_enabled;
            emit ScanAvailabilityChanged();
          });
  setCentralWidget(tabs);
  SetWorkersAvailable(false);
}

void MainWindow::SetWorkersAvailable(bool available) {
  const bool lost_workers = workers_available_ && !available;
  workers_available_ = available;
  if (!available) {
    motor_connections_.fill(ui::ConnectionState::kDisconnected);
    laser_connection_ = ui::ConnectionState::kDisconnected;
    motor_operations_.fill(ui::OperationState::kIdle);
    motor_homed_.fill(std::nullopt);
    laser_output_.reset();
    manual_requests_pending_ = false;
  }
  emit ManualControlsEnabled(available && !controls_locked_);
  emit ScanInputsEnabled(!controls_locked_);
  emit ScanAvailabilityChanged();
  if (lost_workers) ShowMessage(tr("Workers are not available."), true);
}

void MainWindow::SetManualRequestsPending(bool pending) {
  manual_requests_pending_ = pending;
  emit ScanAvailabilityChanged();
}

void MainWindow::SetControlsLocked(bool locked) {
  controls_locked_ = locked;
  emit ManualControlsEnabled(workers_available_ && !locked);
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
      ShowMessage(
          tr("Image imported. Set a start pixel before starting the scan."));
    } else {
      ShowError({{},
                 "Import image",
                 result ? "The image has no exposed pixels." : result.error()});
    }
  }
  emit ScanImageChanged(image);
  emit ScanAvailabilityChanged();
}

void MainWindow::SetStartPixel(int x, int y) {
  if (!CanSetStartPixel()) return;
  if (x < 0 || y < 0 || x >= scan_image_size_.width() ||
      y >= scan_image_size_.height()) {
    ShowMessage(tr("Set start: Enter a nonnegative pixel inside the image."),
                true);
    return;
  }
  scan_configuration_.start_pixel = {x, y};
  start_pixel_set_ = true;
  ShowMessage(tr("Start pixel set to (%1, %2).").arg(x).arg(y));
  emit ScanAvailabilityChanged();
}

QStringList MainWindow::ScanStartBlockers() const {
  QStringList reasons;
  if (!workers_available_) reasons << tr("Workers are unavailable.");
  if (controls_locked_ || scan_state_.phase != ui::ScanPhase::kIdle)
    reasons << tr("Wait until the scan is idle and controls are unlocked.");
  if (manual_requests_pending_)
    reasons << tr("Wait for pending manual commands to finish.");
  const QStringList axes{"X", "Y", "Z"};
  for (std::size_t i = 0; i < motor_connections_.size(); ++i) {
    if (motor_connections_[i] != ui::ConnectionState::kConnected)
      reasons << tr("Connect the %1 motor.").arg(axes.at(i));
    else if (motor_operations_[i] != ui::OperationState::kIdle)
      reasons << tr("Wait until the %1 axis is idle.").arg(axes.at(i));
    else if (i < 2 && !motor_homed_[i].value_or(false))
      reasons << tr("Home the %1 axis before starting a scan.").arg(axes.at(i));
  }
  if (laser_connection_ != ui::ConnectionState::kConnected)
    reasons << tr("Connect the laser.");
  else if (!laser_output_)
    reasons << tr("Confirm the laser output is OFF; its state is unknown.");
  else if (*laser_output_)
    reasons << tr("Turn the laser output OFF.");
  if (!has_pattern_) reasons << tr("Import an image with exposed pixels.");
  if (!std::isfinite(scan_configuration_.pixel_size_mm) ||
      scan_configuration_.pixel_size_mm <= 0)
    reasons << tr("Enter a positive pixel size in micrometres per pixel.");
  if (!start_pixel_set_ ||
      scan_configuration_.start_pixel.x >= scan_image_size_.width() ||
      scan_configuration_.start_pixel.y >= scan_image_size_.height())
    reasons << tr("Apply a valid starting pixel using Set start.");
  return reasons;
}

bool MainWindow::CanStartScan() const { return ScanStartBlockers().isEmpty(); }

void MainWindow::SetPixelSizeMicrometres(double value) {
  if (controls_locked_ || scan_state_.phase != ui::ScanPhase::kIdle) return;
  scan_configuration_.pixel_size_mm = value / 1000.0;
  emit ScanAvailabilityChanged();
}

void MainWindow::RequestScan() {
  if (!CanStartScan()) return;
  position_warning_visible_ = false;
  emit StartScanRequested(scan_configuration_);
}

void MainWindow::UpdateScanState(ui::ScanState state) {
  scan_state_ = state;
  if (state.phase != ui::ScanPhase::kIdle) scan_has_run_ = true;
  emit ScanDisplayChanged(state);
  emit ScanAvailabilityChanged();
}
bool MainWindow::CanResetScan() const {
  using ui::ScanPhase;
  return workers_available_ && !manual_requests_pending_ &&
         (scan_state_.phase == ScanPhase::kFailed ||
          scan_state_.phase == ScanPhase::kPaused ||
          (scan_has_run_ && scan_state_.phase == ScanPhase::kIdle &&
           !controls_locked_));
}
void MainWindow::RequestResetScan() {
  if (!CanResetScan()) return;
  position_warning_visible_ = false;
  ShowMessage(tr("Resetting: turning laser OFF and stopping motors..."));
  emit ResetScanRequested();
}
void MainWindow::OnScanResetCompleted() {
  position_warning_visible_ = false;
  scan_has_run_ = false;
  start_pixel_set_ = false;
  emit ScanAvailabilityChanged();
  ShowMessage(
      tr("Reset complete: laser OFF, motors stopped, manual controls "
         "available. Set the starting pixel before scanning."));
}

void MainWindow::ShowMessage(const QString& message, bool error) {
  // Keep the warning visible through routine polling/scan completion messages.
  if (position_warning_visible_ && !error) return;
  if (error) position_warning_visible_ = false;
  emit StatusMessageChanged(message, error);
}

void MainWindow::ShowWarning(ui::OperationError warning) {
  position_warning_visible_ = true;
  const QString axis = warning.axis ? tr("Axis %1: ")
                                          .arg(QStringList{"X", "Y", "Z"}.at(
                                              static_cast<int>(*warning.axis)))
                                    : QString();
  emit StatusWarningChanged(tr("Warning — ") + axis +
                            QString::fromStdString(warning.message));
}

void MainWindow::ShowError(ui::OperationError error) {
  emit OperationErrorReported(error);
  const QString axis = error.axis ? tr("Axis %1: ")
                                        .arg(QStringList{"X", "Y", "Z"}.at(
                                            static_cast<int>(*error.axis)))
                                  : QString();
  ShowMessage(axis + QString::fromStdString(error.operation) + ": " +
                  QString::fromStdString(error.message),
              true);
}
}  // namespace ui
