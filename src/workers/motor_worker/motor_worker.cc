#include "motor_worker.h"

#include <cmath>
#include <utility>

namespace workers {
namespace {
constexpr int kPollIntervalMs = thorlabs::kDefaultPollingIntervalMs;
// A full 25 mm move at the default 0.01 mm/s takes about 42 minutes.
constexpr int kMotionTimeoutMs = 60 * 60 * 1000;
constexpr int kStopTimeoutMs = 5000;

bool IsMoving(const thorlabs::MotorStatus& status) {
  return status.homing || status.moving_forward || status.moving_reverse ||
         status.jogging_forward || status.jogging_reverse;
}

std::shared_ptr<const thorlabs::KinesisSimulation> SimulationSession(
    bool enabled) {
  if (!enabled) return nullptr;
  // Simulation is process-wide; disconnecting one axis must not stop the
  // others.
  static const auto session = std::make_shared<thorlabs::KinesisSimulation>();
  return session;
}
}  // namespace

MotorWorker::MotorWorker(QObject* parent) : QObject(parent) {}
MotorWorker::~MotorWorker() = default;

bool MotorWorker::CheckReady(RequestId id) {
  if (shutting_down_ || !motor_) {
    emit RequestFinished(
        id, errors::Error::Failure(errors::ErrorCode::kConnectionError,
                                   "Motor is disconnected or shutting down."));
    return false;
  }
  if (pending_motion_ || pending_stop_) {
    emit RequestFinished(
        id, errors::Error::Failure(errors::ErrorCode::kBusy, "Motor is busy."));
    return false;
  }
  return true;
}

void MotorWorker::ConnectDevice(RequestId id, QString serial_number,
                                bool simulation) {
  if (motor_ || shutting_down_) {
    emit RequestFinished(id, errors::Error::Failure(
                                 errors::ErrorCode::kBusy,
                                 "Disconnect the motor before reconnecting."));
    return;
  }
  serial_number = serial_number.trimmed();
  if (serial_number.isEmpty()) {
    emit RequestFinished(
        id, errors::Error::InvalidArgument("Enter a motor serial number."));
    return;
  }
  auto simulation_session = SimulationSession(simulation);
  auto result = thorlabs::KDC101::CreateMotor(
      serial_number.toStdString(), kPollIntervalMs, simulation_session);
  if (!result) {
    emit RequestFinished(id, result.error());
    return;
  }
  simulation_ = std::move(simulation_session);
  motor_ = std::move(*result);
  if (!poll_timer_) {
    poll_timer_ = new QTimer(this);
    connect(poll_timer_, &QTimer::timeout, this, &MotorWorker::PollDevice);
    motion_timer_ = new QTimer(this);
    motion_timer_->setSingleShot(true);
    connect(motion_timer_, &QTimer::timeout, this, &MotorWorker::OnTimeout);
  }
  poll_timer_->start(kPollIntervalMs);
  PublishState();
  emit RequestFinished(id, PublishSettings());
}

void MotorWorker::DisconnectDevice(RequestId id) {
  if (pending_motion_ || pending_stop_) {
    emit RequestFinished(
        id, errors::Error::Failure(errors::ErrorCode::kBusy,
                                   "Stop the motor before disconnecting."));
    return;
  }
  if (poll_timer_) poll_timer_->stop();
  motor_.reset();
  simulation_.reset();
  emit StateChanged({});
  emit RequestFinished(id, errors::Error::Ok());
}

void MotorWorker::Configure(RequestId id, MotorSettings settings) {
  if (!CheckReady(id)) return;
  // Apply in order and report the first failure. A failure can leave earlier
  // settings applied, so the UI must not claim the complete update succeeded.
  auto result = motor_->SetMoveVelocity(settings.move);
  if (result.ok()) result = motor_->SetJogVelocity(settings.jog);
  if (result.ok() && settings.homing_speed_mm_per_second)
    result = motor_->SetHomingSpeed(*settings.homing_speed_mm_per_second);
  if (result.ok() && settings.jog_step_mm)
    result = motor_->SetJogStepSize(*settings.jog_step_mm);
  if (result.ok() && settings.backlash_mm)
    result = motor_->SetBacklash(*settings.backlash_mm);
  // Read back even after a partial update; show device values, not requested
  // ones.
  const auto read_result = PublishSettings();
  if (result.ok()) result = read_result;
  emit RequestFinished(id, result);
}

errors::Error MotorWorker::PublishSettings() {
  auto configuration = motor_->GetConfiguration();
  MotorSettings settings;
  if (configuration) {
    settings.move.acceleration_mm_per_second_squared =
        configuration->acceleration_mm_per_second_squared;
    settings.homing_speed_mm_per_second =
        configuration->homing_speed_mm_per_second;
    settings.backlash_mm = configuration->backlash_mm;
  }
  emit SettingsChanged(settings);
  return configuration ? errors::Error::Ok() : configuration.error();
}

void MotorWorker::StartMotion(RequestId id, thorlabs::MotorEvent completion,
                              std::function<errors::Error()> command,
                              bool continuous) {
  if (!CheckReady(id)) return;
  auto result = motor_->ClearMessageQueue();
  if (result.ok()) result = command();
  if (!result.ok()) {
    emit RequestFinished(id, result);
    return;
  }
  pending_motion_ = PendingMotion{id, completion};
  if (!continuous) motion_timer_->start(kMotionTimeoutMs);
  PublishState();
}

void MotorWorker::Home(RequestId id) {
  StartMotion(id, thorlabs::MotorEvent::kHomed,
              [this] { return motor_->StartHome(); });
}
void MotorWorker::MoveAbsolute(RequestId id, double position_mm) {
  if (motor_ && !pending_motion_ && !pending_stop_ &&
      std::isfinite(position_mm)) {
    auto position = motor_->GetPosition();
    auto resolution = motor_->GetDistanceResolution();
    if (position && resolution &&
        std::abs(*position - position_mm) < *resolution / 2) {
      emit RequestFinished(id, errors::Error::Ok());
      return;
    }
  }
  StartMotion(id, thorlabs::MotorEvent::kMoveCompleted, [this, position_mm] {
    return motor_->StartMoveAbsolute(position_mm);
  });
}
void MotorWorker::MoveRelative(RequestId id, double distance_mm) {
  StartMotion(id, thorlabs::MotorEvent::kMoveCompleted, [this, distance_mm] {
    return motor_->StartMoveRelative(distance_mm);
  });
}
void MotorWorker::Jog(RequestId id, thorlabs::Direction direction) {
  StartMotion(id, thorlabs::MotorEvent::kMoveCompleted,
              [this, direction] { return motor_->StartJog(direction); });
}
void MotorWorker::Drive(RequestId id, thorlabs::Direction direction) {
  StartMotion(
      id, thorlabs::MotorEvent::kStopped,
      [this, direction] { return motor_->StartDrive(direction); }, true);
}

void MotorWorker::Stop(RequestId id, thorlabs::StopMode mode) {
  if (!motor_) {
    emit RequestFinished(id, errors::Error::Ok());
    return;
  }
  if (pending_stop_) {
    emit RequestFinished(id,
                         errors::Error::Failure(errors::ErrorCode::kBusy,
                                                "Stop is already pending."));
    return;
  }
  auto result = motor_->Stop(mode);
  if (!result.ok()) {
    emit RequestFinished(id, result);
    return;
  }
  pending_stop_ = id;
  stop_idle_polls_ = 0;
  motion_timer_->start(kStopTimeoutMs);
  // Wait for polling rather than trusting cached status immediately after Stop.
}

void MotorWorker::FinishMotion(errors::Error result) {
  if (!pending_motion_) return;
  const auto id = pending_motion_->id;
  pending_motion_.reset();
  if (!pending_stop_) motion_timer_->stop();
  emit RequestFinished(id, result);
}
void MotorWorker::FinishStop(errors::Error result) {
  if (!pending_stop_) return;
  const auto id = *pending_stop_;
  pending_stop_.reset();
  motion_timer_->stop();
  const bool normal_drive_stop =
      pending_motion_ &&
      pending_motion_->completion_event == thorlabs::MotorEvent::kStopped;
  FinishMotion(result.ok() && !normal_drive_stop
                   ? errors::Error::Failure(errors::ErrorCode::kCancelled,
                                            "Motion stopped.")
                   : result);
  emit RequestFinished(id, result);
}

void MotorWorker::PublishState() {
  if (!motor_) {
    emit StateChanged({});
    return;
  }
  MotorState state;
  auto status = motor_->GetStatus();
  auto position = motor_->GetPosition();
  state.connected = status.has_value();
  if (status) state.status = *status;
  if (position) state.position_mm = *position;
  emit StateChanged(state);
}

void MotorWorker::PollDevice() {
  if (!motor_) return;
  auto status = motor_->GetStatus();
  if (!status) {
    const auto error = status.error();
    poll_timer_->stop();
    PublishState();
    FinishMotion(error);
    FinishStop(error);
    motor_.reset();
    simulation_.reset();
    emit StateChanged({});
    emit PollingFailed(error);
    return;
  }
  PublishState();
  if (pending_stop_) {
    stop_idle_polls_ = IsMoving(*status) ? 0 : stop_idle_polls_ + 1;
    // Already-idle motors may not emit a stopped event. Confirm idle across
    // two polling ticks before acknowledging the stop request.
    if (stop_idle_polls_ >= 2) FinishStop(errors::Error::Ok());
  }
  // Bound queue draining so other queued commands, especially Stop, can run.
  for (int i = 0; i < 64; ++i) {
    auto event = motor_->GetNextEvent();
    if (!event) {
      FinishMotion(event.error());
      FinishStop(event.error());
      emit PollingFailed(event.error());
      return;
    }
    if (!*event) break;
    if (pending_stop_)
      continue;  // Confirm stopping through status, not a stale event.
    if (!pending_motion_) continue;
    if (**event == pending_motion_->completion_event)
      FinishMotion(errors::Error::Ok());
    else if (**event == thorlabs::MotorEvent::kStopped)
      FinishMotion(
          errors::Error::Failure(errors::ErrorCode::kCancelled,
                                 "Motor stopped before completing the move."));
  }
}

void MotorWorker::OnTimeout() {
  auto error = errors::Error::Failure(errors::ErrorCode::kTimeout,
                                      "Motor operation timed out.");
  if (motor_) {
    auto stop = motor_->Stop(thorlabs::StopMode::kImmediate);
    if (!stop.ok()) emit PollingFailed(stop);
  }
  FinishMotion(error);
  FinishStop(error);
  emit PollingFailed(error);
}

void MotorWorker::Shutdown() {
  if (shutting_down_) return;
  shutting_down_ = true;
  if (poll_timer_) poll_timer_->stop();
  if (motion_timer_) motion_timer_->stop();
  if (motor_) {
    auto result = motor_->Stop(thorlabs::StopMode::kImmediate);
    if (!result.ok()) emit PollingFailed(result);
  }
  const auto cancelled = errors::Error::Failure(errors::ErrorCode::kCancelled,
                                                "Motor worker shut down.");
  FinishMotion(cancelled);
  FinishStop(cancelled);
  motor_.reset();
  simulation_.reset();
  emit StateChanged({});
  emit ShutdownFinished();
}
}  // namespace workers
