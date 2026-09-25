#ifndef FRONTEND_MAIN_WINDOW_H
#define FRONTEND_MAIN_WINDOW_H

#include <array>
#include <QStringList>
#include <QMainWindow>
#include <QImage>

#include "../../application/application_types/application_types.h"

namespace ui {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  void SetBackendAvailable(bool available);
  void SetScanImage(const QImage& image);
  void SetStartPixel(int x, int y);
  bool CanStartScan() const;
  QStringList ScanStartBlockers() const;
  bool CanSetStartPixel() const { return !controls_locked_ && !scan_image_size_.isEmpty(); }
  bool HasBackend() const { return backend_available_; }
  application::ScanPhase ScanPhase() const { return scan_state_.phase; }

 public slots:
  void SetControlsLocked(bool locked);
  void SetManualRequestsPending(bool pending);
  void UpdateScanState(application::ScanState state);
  void ShowMessage(const QString& message, bool error = false);
  void ShowError(application::OperationError error);
  void RequestScan();

 signals:
  // User intent. Application decides whether requests can be serviced.
  void ConnectMotorRequested(application::MotorConnection connection);
  void DisconnectMotorRequested(application::Axis axis);
  void ConfigureAxisRequested(application::Axis axis, application::MotorSettings settings);
  void HomeAxisRequested(application::Axis axis);
  void MoveAxisRequested(application::Axis axis, double position_mm);
  void JogAxisRequested(application::Axis axis, application::Direction direction);
  void DriveAxisRequested(application::Axis axis, application::Direction direction);
  void StopAxisRequested(application::Axis axis, application::StopMode mode);
  void ConnectLaserRequested(application::LaserConnection connection);
  void DisconnectLaserRequested();
  void SetLaserOutputRequested(bool enabled);
  void StartScanRequested(application::ScanConfiguration configuration);
  void PauseScanRequested();
  void ResumeScanRequested();
  void CancelScanRequested();

  // Broadcast application observations to the display sections.
  void StatusMessageChanged(const QString& message, bool error);
  void AxisStateUpdated(application::AxisState state);
  void LaserStateUpdated(application::LaserState state);
  void ScanDisplayChanged(application::ScanState state);
  void ManualControlsEnabled(bool enabled);
  void ScanInputsEnabled(bool enabled);
  void ScanAvailabilityChanged();

 private:
  std::array<application::ConnectionState, 3> motor_connections_{};
  application::ConnectionState laser_connection_ = application::ConnectionState::kDisconnected;
  std::array<application::OperationState, 3> motor_operations_{};
  std::optional<bool> laser_output_;
  bool manual_requests_pending_ = false;
  bool backend_available_ = false;
  bool controls_locked_ = false;
  bool has_pattern_ = false;
  bool start_pixel_set_ = false;
  QSize scan_image_size_;
  application::ScanConfiguration scan_configuration_;
  application::ScanState scan_state_;
};

}  // namespace ui

#endif  // FRONTEND_MAIN_WINDOW_H
