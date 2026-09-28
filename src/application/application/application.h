#ifndef APPLICATION_APPLICATION_H_
#define APPLICATION_APPLICATION_H_

#include <array>
#include <memory>
#include <optional>
#include <unordered_set>
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
  bool UsesSimulatedMotors() const { return simulation_->IsSimulation(); }
  bool HasPendingAxisRequest(Axis axis) const {
    const auto i = static_cast<unsigned>(axis);
    return i < 3 && (axis_requests_[i] || axis_stops_[i]);
  }
  bool HasPendingAxisStop(Axis axis) const {
    const auto i = static_cast<unsigned>(axis);
    return i < 3 && axis_stops_[i];
  }
  bool HasPendingLaserRequest() const { return laser_request_ != 0; }
  bool HasPendingManualRequests() const { return !pending_operations_.empty(); }

// UI exposed API to interact with hardware
 public slots:
  void ConnectMotor(MotorConnection connection);
  void DisconnectMotor(Axis axis);
  void ConfigureAxis(Axis axis, MotorSettings settings);
  void HomeAxis(Axis axis);
  void MoveAxis(Axis axis, double position_mm);
  void JogAxis(Axis axis, Direction direction);
  void DriveAxis(Axis axis, Direction direction);
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
  void AxisSettingsApplied(application::Axis axis, application::MotorSettings settings);
  void AxisStateUpdated(application::AxisState state);
  void LaserStateUpdated(application::LaserState state);
  
  void ControlsLocked(bool locked);
  void ManualRequestsPending(bool pending);
  void LaserRequestPending(bool pending);
  void AxisRequestsPending(application::Axis axis, bool pending, bool stopping);
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
  
  int laser_request_ = 0;
  std::unordered_set<int> pending_operations_;
  std::unordered_map<int, std::pair<Axis, MotorSettings>> pending_settings_;

  [[nodiscard]] int NextRequestId();
  int next_request_id_ = 1;
  
  void FinishManualRequest(int id);
  std::optional<OperationError> ScanReadinessError() const;
  int BeginManualRequest(std::optional<Axis> axis, bool stop = false);
  std::array<int, 3> axis_requests_{};
  std::array<int, 3> axis_stops_{};

  std::array<ConnectionState, 3> motor_connections_{};
  std::array<OperationState, 3> motor_operations_{};
  std::array<std::optional<bool>, 3> motor_homed_{};
  std::optional<bool> laser_output_;
  ConnectionState laser_connection_ = ConnectionState::kDisconnected;
  // Outlives every motor and its worker thread. All axes share one SDK session.
  std::shared_ptr<const thorlabs::KinesisSimulation> simulation_;
  std::array<std::unique_ptr<MotorController>, 3> motors_;
  std::unique_ptr<LaserController> laser_;
  std::unique_ptr<ScanController> scan_;
};

}  // namespace application

#endif  // APPLICATION_APPLICATION_H_
