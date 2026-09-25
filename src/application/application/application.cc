#include "application.h"

#include <climits>
#include <utility>

namespace application {
Application::Application(QObject* parent) : QObject(parent) {
  qRegisterMetaType<AxisState>();
  qRegisterMetaType<LaserState>();
  qRegisterMetaType<ScanState>();
  qRegisterMetaType<OperationError>();
  for (int i = 0; i < 3; ++i) {
    motors_[i] = std::make_unique<MotorController>(static_cast<Axis>(i));
    auto* motor = motors_[i].get();
    connect(motor, &MotorController::StateChanged, this, [this, i](AxisState state) {
      motor_connections_[i] = state.connection;
      motor_operations_[i] = state.operation;
      emit AxisStateUpdated(state);
    });
    connect(motor, &MotorController::RequestCompleted, this, &Application::OnWorkerCompleted);
    connect(motor, &MotorController::RequestFailed, this, &Application::OnWorkerFailed);
    connect(motor, &MotorController::RequestCancelled, this, &Application::OnWorkerCancelled);
  }
  laser_ = std::make_unique<LaserController>();
  connect(laser_.get(), &LaserController::StateChanged, this, [this](LaserState state) {
    laser_connection_ = state.connection;
    laser_output_ = state.output_enabled;
    emit LaserStateUpdated(state);
  });
  connect(laser_.get(), &LaserController::RequestCompleted, this, &Application::OnWorkerCompleted);
  connect(laser_.get(), &LaserController::RequestFailed, this, &Application::OnWorkerFailed);
  scan_ = std::make_unique<ScanController>(*motors_[0], *motors_[1], *motors_[2], *laser_);
  connect(scan_.get(), &ScanController::StateChanged, this, &Application::ScanStateUpdated);
  connect(scan_.get(), &ScanController::RequestFailed, this, &Application::RequestFailed);
  connect(scan_.get(), &ScanController::ScanCompleted, this, &Application::OnScanCompleted);
  connect(scan_.get(), &ScanController::ScanCancelled, this, &Application::OnScanCancelled);
  connect(scan_.get(), &ScanController::ScanFailed, this, &Application::OnScanFailed);
}
Application::~Application() {
  // Destroy the scan first; its borrowed controllers remain alive during teardown.
  scan_.reset();
  laser_.reset();  // Attempt OFF before releasing the motors.
  for (auto& motor : motors_) motor.reset();
}
int Application::NextRequestId() {
  const int id = next_request_id_;
  if (id) next_request_id_ = id == INT_MAX ? 0 : id + 1;
  return id;
}
int Application::BeginManualRequest(std::optional<Axis> axis) {
  if (axis && (*axis < Axis::kX || *axis > Axis::kZ)) {
    emit RequestFailed({{}, "Request", "Invalid axis."}); return 0;
  }
  if (state_ != ApplicationState::kManual) {
    emit RequestFailed({axis, "Request", "Manual controls are locked."}); return 0;
  }
  const int id = NextRequestId();
  if (!id) { emit RequestFailed({axis, "Request", "Request IDs exhausted."}); return 0; }
  const bool was_empty = pending_operations_.empty();
  pending_operations_.insert(id);
  if (was_empty) emit ManualRequestsPending(true);
  return id;
}
void Application::ConnectMotor(MotorConnection connection) {
  if (int id = BeginManualRequest(connection.axis)) motors_[static_cast<int>(connection.axis)]->Connect(id, connection);
}
void Application::DisconnectMotor(Axis axis) {
  if (int id = BeginManualRequest(axis)) motors_[static_cast<int>(axis)]->Disconnect(id);
}
void Application::ConfigureAxis(Axis axis, MotorSettings settings) {
  if (int id = BeginManualRequest(axis)) motors_[static_cast<int>(axis)]->ConfigureMotion(id, settings);
}
void Application::HomeAxis(Axis axis) {
  if (int id = BeginManualRequest(axis)) motors_[static_cast<int>(axis)]->Home(id);
}
void Application::MoveAxis(Axis axis, double position) {
  if (int id = BeginManualRequest(axis)) motors_[static_cast<int>(axis)]->MoveAbsolute(id, position);
}
void Application::JogAxis(Axis axis, Direction direction) {
  if (int id = BeginManualRequest(axis)) motors_[static_cast<int>(axis)]->Jog(id, direction);
}
void Application::DriveAxis(Axis axis, Direction direction) {
  if (int id = BeginManualRequest(axis)) motors_[static_cast<int>(axis)]->Drive(id, direction);
}
void Application::StopAxis(Axis axis, StopMode mode) {
  if (int id = BeginManualRequest(axis)) motors_[static_cast<int>(axis)]->Stop(id, mode);
}
void Application::ConnectLaser(LaserConnection connection) {
  if (int id = BeginManualRequest({})) laser_->Connect(id, connection);
}
void Application::DisconnectLaser() {
  if (int id = BeginManualRequest({})) laser_->Disconnect(id);
}
void Application::SetLaserOutput(bool enabled) {
  if (int id = BeginManualRequest({})) laser_->SetOutputEnabled(id, enabled);
}
std::optional<OperationError> Application::ScanReadinessError() const {
  if (HasPendingManualRequests())
    return OperationError{{}, "Start scan", "Wait for pending manual commands to finish."};
  for (std::size_t i = 0; i < motor_connections_.size(); ++i) {
    if (motor_connections_[i] != ConnectionState::kConnected)
      return OperationError{static_cast<Axis>(i), "Start scan", "Connect all three motors before starting a scan."};
    if (motor_operations_[i] != OperationState::kIdle)
      return OperationError{static_cast<Axis>(i), "Start scan", "Wait until all axes are idle."};
  }
  if (laser_connection_ != ConnectionState::kConnected)
    return OperationError{{}, "Start scan", "Connect the laser before starting a scan."};
  if (!laser_output_ || *laser_output_)
    return OperationError{{}, "Start scan", "Confirm the laser output is OFF before starting a scan."};
  return {};
}

void Application::StartScan(ScanConfiguration configuration) {
  if (state_ != ApplicationState::kManual) {
    emit RequestFailed({{}, "Start scan", "Application is already reserved or failed."}); return;
  }
  if (const auto error = ScanReadinessError()) {
    emit RequestFailed(*error);
    return;
  }
  state_ = ApplicationState::kScanRequested;
  auto result = scan_->Configure(std::move(configuration));
  if (!result) {
    state_ = ApplicationState::kManual;
    emit ControlsLocked(false);
    emit RequestFailed(result.error());
    return;
  }
  // Configure can notify a caller that cancels immediately.
  if (state_ != ApplicationState::kScanRequested) return;
  emit ControlsLocked(true);
  // A lock notification may synchronously cancel the configured scan.
  if (state_ != ApplicationState::kScanRequested) return;
  state_ = ApplicationState::kScanning;
  scan_->Start();
}
void Application::PauseScan() { scan_->Pause(); }
void Application::ResumeScan() { scan_->Resume(); }
void Application::CancelScan() { scan_->Cancel(); }
void Application::FinishManualRequest(int id) {
  if (pending_operations_.erase(id) && pending_operations_.empty())
    emit ManualRequestsPending(false);
}
void Application::OnWorkerCompleted(int id) {
  FinishManualRequest(id);
}
void Application::OnWorkerFailed(int id, OperationError error) {
  // ScanController handles unsolicited faults during its execution.
  if (id == 0 && state_ == ApplicationState::kScanning) return;
  if (id != 0 && !pending_operations_.contains(id)) return;
  FinishManualRequest(id);
  emit RequestFailed(error);
}
void Application::OnWorkerCancelled(int id) {
  FinishManualRequest(id);
}
void Application::OnScanCompleted() {
  state_ = ApplicationState::kManual;
  emit ControlsLocked(false);
}
void Application::OnScanCancelled() {
  state_ = ApplicationState::kManual;
  emit ControlsLocked(false);
}
void Application::OnScanFailed(OperationError error) {
  state_ = ApplicationState::kFailure;
  emit ControlsLocked(true);
  emit RequestFailed(error);
}
}  // namespace application
