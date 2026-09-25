#ifndef APPLICATION_APPLICATION_H_
#define APPLICATION_APPLICATION_H_

#include <array>
#include <memory>
#include <optional>
#include <unordered_map>

#include <QObject>

#include "../application_types/application_types.h"
#include "../laser_controller/laser_controller.h"
#include "../motor_controller/motor_controller.h"
#include "../scan_controller/scan_controller.h"

namespace application {

// Central interface to connect the UI to the hardware
class Application : public QObject {
  Q_OBJECT

 public:
  explicit Application(QObject* parent = nullptr);
  ~Application() override;

// UI exposed API to interact with hardware
 public slots:
  void ConnectMotor(MotorConnection connection);
  void DisconnectMotor(Axis axis);
  void HomeAxis(Axis axis);
  void MoveAxis(Axis axis, double position_mm, MotionSettings settings);
  void JogAxis(Axis axis, Direction direction, double step_mm, MotionSettings settings);
  void DriveAxis(Axis axis, Direction direction, MotionSettings settings);
  void StopAxis(Axis axis, StopMode mode);

  void ConnectLaser(LaserConnection connection);
  void DisconnectLaser();
  void SetLaserOutput(bool enabled);

  void StartScan(ScanConfiguration configuration);
  void PauseScan();
  void ResumeScan();
  void CancelScan();

// Notifications sent back to the UI
 signals:
  void AxisStateUpdated(application::AxisState state);
  void LaserStateUpdated(application::LaserState state);
  
  void ControlsLocked(bool locked);
  void ScanStateUpdated(application::ScanState state);
  void RequestFailed(application::OperationError error);

// Internal functions to handle worker signals
 private slots:
  void OnWorkerCompleted(int id);
  void OnWorkerFailed(int id, OperationError error);
  void OnWorkerCancelled(int id);
  void OnScanCompleted();
  void OnScanCancelled();
  void OnScanFailed(OperationError error);

 private:
  enum class ApplicationState { kManual, kScanRequested, kScanning, kFailure };
  ApplicationState state_ = ApplicationState::kManual;
  
  // Tracks an pending, unfinished command
  struct PendingOperation {
    std::optional<Axis> axis;
    bool continuous_drive = false;
  };

  std::unordered_map<int, PendingOperation> pending_operations_;

  [[nodiscard]] int NextRequestId();
  int next_request_id_ = 1;
  
  void TryStartScan();
  int BeginManualRequest(std::optional<Axis> axis, bool continuous_drive = false);

  std::array<std::unique_ptr<MotorController>, 3> motors_;
  std::unique_ptr<LaserController> laser_;
  std::unique_ptr<ScanController> scan_;
};

}  // namespace application

#endif  // APPLICATION_APPLICATION_H_
