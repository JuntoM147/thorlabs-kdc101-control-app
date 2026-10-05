#ifndef FRONTEND_MAIN_WINDOW_H
#define FRONTEND_MAIN_WINDOW_H

#include <QImage>
#include <QMainWindow>
#include <QStringList>
#include <array>
#include <functional>
#include <map>

#include "ui_types.h"
#include "workers/worker_manager/worker_manager.h"

namespace ui {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  void ConnectWorkers(workers::WorkerManager& workers);
  void SetWorkersAvailable(bool available);
  void SetScanImage(const QImage& image);
  void SetStartPixel(int x, int y);
  void SetPixelSizeMicrometres(double value);
  double PixelSizeMicrometres() const {
    return scan_configuration_.pixel_size_mm * 1000.0;
  }
  bool CanStartScan() const;
  bool CanResetScan() const;
  QStringList ScanStartBlockers() const;
  bool CanSetStartPixel() const {
    return !controls_locked_ && !scan_image_size_.isEmpty();
  }
  bool HasWorkers() const { return workers_available_; }
  ui::ScanPhase ScanPhase() const { return scan_state_.phase; }

 public slots:
  void SetControlsLocked(bool locked);
  void SetManualRequestsPending(bool pending);
  void UpdateScanState(ui::ScanState state);
  void ShowMessage(const QString& message, bool error = false);
  void ShowError(ui::OperationError error);
  void ShowWarning(ui::OperationError warning);
  void RequestScan();
  void RequestResetScan();
  void OnScanResetCompleted();

 signals:
  // Widget requests are checked by the window and queued to workers.
  void ConnectMotorRequested(ui::MotorConnection connection);
  void DisconnectMotorRequested(ui::Axis axis);
  void ConfigureAxisRequested(ui::Axis axis, ui::MotorSettings settings);
  void HomeAxisRequested(ui::Axis axis);
  void MoveAxisRequested(ui::Axis axis, double position_mm);
  void JogAxisRequested(ui::Axis axis, ui::Direction direction);
  void DriveAxisRequested(ui::Axis axis, ui::Direction direction);
  void StopAxisRequested(ui::Axis axis, ui::StopMode mode);
  void ConnectLaserRequested(ui::LaserConnection connection);
  void DisconnectLaserRequested();
  void SetLaserOutputRequested(bool enabled);
  void StartScanRequested(ui::ScanConfiguration configuration);
  void PauseScanRequested();
  void ResumeScanRequested();
  void CancelScanRequested();
  void ResetScanRequested();

  // Broadcast worker observations to the display sections.
  void StatusMessageChanged(const QString& message, bool error);
  void StatusWarningChanged(const QString& message);
  void OperationErrorReported(ui::OperationError error);
  void AxisSettingsApplied(ui::Axis axis, ui::MotorSettings settings);
  void AxisSettingsUpdated(ui::Axis axis, ui::MotorSettings settings);
  void AxisStateUpdated(ui::AxisState state);
  void AxisRequestsPending(ui::Axis axis, bool pending, bool stopping);
  void LaserStateUpdated(ui::LaserState state);
  void LaserRequestPending(bool pending);
  void ScanDisplayChanged(ui::ScanState state);
  void ManualControlsEnabled(bool enabled);
  void ScanInputsEnabled(bool enabled);
  void ScanAvailabilityChanged();
  void ScanImageChanged(const QImage& image);

 private:
  struct PendingRequest {
    std::optional<Axis> axis;
    std::string operation;
    std::optional<MotorSettings> settings;
    bool stopping = false;
  };
  void ConnectMotorWorker(Axis axis, workers::MotorWorker* motor);
  void ConnectLaserWorker(workers::LaserWorker* laser);
  void ConnectScanWorker(workers::ScanWorker* scan);
  void ConnectScanAxis(workers::ScanWorker* scan, Axis axis,
                       workers::MotorWorker* motor);
  void SubmitMotor(
      Axis axis, std::string operation,
      std::function<void(workers::MotorWorker*, workers::RequestId)> command,
      std::optional<MotorSettings> settings = std::nullopt,
      bool stopping = false);
  void SubmitLaser(
      std::string operation,
      std::function<void(workers::LaserWorker*, workers::RequestId)> command);
  void FinishRequest(workers::RequestId id, errors::Error result);
  void UpdatePendingRequests();
  void PublishMotorState(Axis axis, workers::MotorState state);
  workers::WorkerManager* workers_ = nullptr;
  // Manual IDs use the upper half; scan IDs use the lower half.
  workers::RequestId next_request_id_ = workers::RequestId{1} << 63;
  std::map<workers::RequestId, PendingRequest> pending_requests_;
  bool reset_requested_ = false;
  bool reset_failed_ = false;
  std::array<ui::ConnectionState, 3> motor_connections_{};
  ui::ConnectionState laser_connection_ = ui::ConnectionState::kDisconnected;
  std::array<ui::OperationState, 3> motor_operations_{};
  std::array<std::optional<bool>, 3> motor_homed_{};
  std::optional<bool> laser_output_;
  bool manual_requests_pending_ = false;
  bool workers_available_ = false;
  bool controls_locked_ = false;
  bool has_pattern_ = false;
  bool start_pixel_set_ = false;
  bool scan_has_run_ = false;
  bool position_warning_visible_ = false;
  QSize scan_image_size_;
  ui::ScanConfiguration scan_configuration_;
  ui::ScanState scan_state_;
};

}  // namespace ui

#endif  // FRONTEND_MAIN_WINDOW_H
