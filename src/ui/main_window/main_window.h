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
  void SetPixelSizeMicrometres(double value);
  double PixelSizeMicrometres() const { return scan_configuration_.pixel_size_mm * 1000.0; }
  void SetRoutePreviewVisible(bool visible);
  void ResetRoute();
  bool CanPreviewRoute() const { return CanResetRoute() && has_pattern_; }
  bool CanResetRoute() const {
    return CanSetStartPixel() && start_pixel_set_ && scan_state_.phase == application::ScanPhase::kIdle;
  }
  bool RoutePreviewVisible() const { return !preview_program_.empty(); }
  const algo::Program& PreviewProgram() const { return preview_program_; }
  algo::PixelPosition StartPixel() const { return scan_configuration_.start_pixel; }
  bool CanStartScan() const;
  bool CanResetScan() const;
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
  void ShowWarning(application::OperationError warning);
  void RequestScan();
  void RequestResetScan();
  void OnScanResetCompleted();

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
  void ResetScanRequested();

  // Broadcast application observations to the display sections.
  void StatusMessageChanged(const QString& message, bool error);
  void StatusWarningChanged(const QString& message);
  void OperationErrorReported(application::OperationError error);
  void AxisSettingsApplied(application::Axis axis, application::MotorSettings settings);
  void AxisStateUpdated(application::AxisState state);
  void AxisRequestsPending(application::Axis axis, bool pending, bool stopping);
  void LaserStateUpdated(application::LaserState state);
  void LaserRequestPending(bool pending);
  void ScanDisplayChanged(application::ScanState state);
  void ManualControlsEnabled(bool enabled);
  void ScanInputsEnabled(bool enabled);
  void ScanAvailabilityChanged();
  void ScanImageChanged(const QImage& image);
  void RoutePreviewChanged();

 private:
  std::array<application::ConnectionState, 3> motor_connections_{};
  application::ConnectionState laser_connection_ = application::ConnectionState::kDisconnected;
  std::array<application::OperationState, 3> motor_operations_{};
  std::array<std::optional<bool>, 3> motor_homed_{};
  std::optional<bool> laser_output_;
  bool manual_requests_pending_ = false;
  bool backend_available_ = false;
  bool controls_locked_ = false;
  bool has_pattern_ = false;
  bool start_pixel_set_ = false;
  bool scan_has_run_ = false;
  bool position_warning_visible_ = false;
  QSize scan_image_size_;
  algo::Program preview_program_;
  application::ScanConfiguration scan_configuration_;
  application::ScanState scan_state_;
};

}  // namespace ui

#endif  // FRONTEND_MAIN_WINDOW_H
