#include "motor_worker.h"

#include <cmath>
#include <utility>

namespace application {
namespace {
bool Moving(const thorlabs::MotorStatus& status) {
  return status.homing || status.moving_forward || status.moving_reverse ||
         status.jogging_forward || status.jogging_reverse;
}
bool Valid(MotionSettings settings) {
  const auto valid = [](std::optional<double> value) {
    return !value || (std::isfinite(*value) && *value > 0);
  };
  return valid(settings.speed_mm_per_second) && valid(settings.acceleration_mm_per_second_squared);
}
}  // namespace

MotorWorker::MotorWorker(Axis axis, QObject* parent) : QObject(parent), axis_(axis) {
  poll_timer_ = new QTimer(this);
  connect(poll_timer_, &QTimer::timeout, this, &MotorWorker::PollDevice);
}
MotorWorker::~MotorWorker() = default;

void MotorWorker::Connect(int id, MotorConnection connection) {
  if (shutting_down_ || motor_ || connection.axis != axis_ || connection.serial_number.empty() ||
      connection.polling_interval_ms <= 0) {
    emit RequestFailed(id, {axis_, "Connect", "Invalid connection or device already connected."});
    return;
  }
  emit StateChanged({axis_, ConnectionState::kConnecting});
  auto result = thorlabs::KDC101::CreateMotor(connection.serial_number, connection.polling_interval_ms);
  if (!result) {
    emit StateChanged({axis_, ConnectionState::kDisconnected});
    emit RequestFailed(id, {axis_, "Connect", result.error().error_message()});
    return;
  }
  motor_ = std::move(*result);
  poll_timer_->start(connection.polling_interval_ms);
  PollDevice();
  if (motor_) emit RequestCompleted(id);
  else emit RequestFailed(id, {axis_, "Connect", "Initial hardware observation failed."});
}

void MotorWorker::Disconnect(int id) {
  if (active_request_ || stop_request_) {
    emit RequestFailed(id, {axis_, "Disconnect", "Stop the motor before disconnecting."});
    return;
  }
  poll_timer_->stop();
  motor_.reset();
  emit StateChanged({axis_, ConnectionState::kDisconnected});
  emit RequestCompleted(id);
}

void MotorWorker::BeginMotion(int id, OperationState operation,
                              const std::function<thorlabs::DeviceStatus()>& start) {
  if (!motor_ || shutting_down_ || active_request_ || stop_request_) {
    emit RequestFailed(id, {axis_, "Motion", "Motor is disconnected or busy."});
    return;
  }
  auto observed = motor_->GetStatus();
  if (!observed || Moving(*observed)) {
    emit RequestFailed(id, {axis_, "Motion", observed ? "Motor is already moving."
                                                      : observed.error().error_message()});
    return;
  }
  auto result = motor_->ClearMessageQueue();
  if (result.ok()) result = start();
  if (!result.ok()) {
    emit RequestFailed(id, {axis_, "Motion", result.error_message()});
    return;
  }
  active_request_ = id;
  operation_ = operation;
  emit RequestAccepted(id);
  if (id > 0 && operation != OperationState::kDriving) {
    QTimer::singleShot(60000, this, [this, id] {
      if (active_request_ == id) FailDevice({axis_, "Motion", "Timed out waiting for completion."});
    });
  }
  PollDevice();
}
void MotorWorker::Home(int id) {
  BeginMotion(id, OperationState::kHoming, [this] { return motor_->StartHome(); });
}
void MotorWorker::MoveAbsolute(int id, double position, MotionSettings settings) {
  if (!std::isfinite(position) || !Valid(settings)) {
    emit RequestFailed(id, {axis_, "Move", "Invalid position or motion settings."}); return;
  }
  BeginMotion(id, OperationState::kMoving, [=, this] {
    return motor_->StartMoveAbsolute(position, settings.speed_mm_per_second.value_or(0),
                                    settings.acceleration_mm_per_second_squared.value_or(0));
  });
}
void MotorWorker::MoveRelative(int id, double distance, MotionSettings settings) {
  if (!std::isfinite(distance) || !Valid(settings)) {
    emit RequestFailed(id, {axis_, "Move", "Invalid distance or motion settings."}); return;
  }
  BeginMotion(id, OperationState::kMoving, [=, this] {
    return motor_->StartMoveRelative(distance, settings.speed_mm_per_second.value_or(0),
                                    settings.acceleration_mm_per_second_squared.value_or(0));
  });
}
void MotorWorker::Jog(int id, Direction direction, double step, MotionSettings settings) {
  if (!std::isfinite(step) || step <= 0 || !Valid(settings) ||
      (direction != Direction::kForward && direction != Direction::kBackward)) {
    emit RequestFailed(id, {axis_, "Jog", "Invalid direction, step or motion settings."}); return;
  }
  BeginMotion(id, OperationState::kJogging, [=, this] {
    return motor_->StartJog(direction == Direction::kForward ? thorlabs::Direction::kForward
                                                            : thorlabs::Direction::kBackward,
                           step, settings.speed_mm_per_second.value_or(0),
                           settings.acceleration_mm_per_second_squared.value_or(0));
  });
}
void MotorWorker::Drive(int id, Direction direction, MotionSettings settings) {
  if (!Valid(settings) || (direction != Direction::kForward && direction != Direction::kBackward)) {
    emit RequestFailed(id, {axis_, "Drive", "Invalid direction or motion settings."}); return;
  }
  BeginMotion(id, OperationState::kDriving, [=, this] {
    return motor_->StartDrive(direction == Direction::kForward ? thorlabs::Direction::kForward
                                                              : thorlabs::Direction::kBackward,
                             settings.speed_mm_per_second.value_or(0),
                             settings.acceleration_mm_per_second_squared.value_or(0));
  });
}
void MotorWorker::Stop(int id, StopMode mode) {
  if (!motor_ || shutting_down_ || stop_request_ ||
      (mode != StopMode::kProfiled && mode != StopMode::kImmediate)) {
    emit RequestFailed(id, {axis_, "Stop", "Motor is disconnected, stopping, or stop mode is invalid."}); return;
  }
  stop_request_ = id;
  auto result = motor_->Stop(mode == StopMode::kProfiled ? thorlabs::StopMode::kProfiled
                                                        : thorlabs::StopMode::kImmediate);
  if (!result.ok()) { FailDevice({axis_, "Stop", result.error_message()}); return; }
  operation_ = OperationState::kStopping;
  QTimer::singleShot(10000, this, [this, id] {
    if (stop_request_ == id) FailDevice({axis_, "Stop", "Motor did not confirm stopped."});
  });
  PollDevice();
}

void MotorWorker::FailDevice(OperationError error) {
  const auto active = std::exchange(active_request_, {});
  const auto stop = std::exchange(stop_request_, {});
  poll_timer_->stop();
  motor_.reset();
  operation_ = OperationState::kIdle;
  emit StateChanged({axis_, ConnectionState::kFaulted});
  if (active) emit RequestFailed(*active, error);
  if (stop) emit RequestFailed(*stop, error);
  if (!active && !stop) emit RequestFailed(0, error);
}

void MotorWorker::PollDevice() {
  if (!motor_ || shutting_down_) return;
  auto status = motor_->GetStatus();
  auto position = motor_->GetPosition();
  if (!status || !position) {
    FailDevice({axis_, "Poll", !status ? status.error().error_message() : position.error().error_message()});
    return;
  }
  bool completed = false;
  bool stopped = false;
  // Bound event draining so a noisy device cannot starve commands or timers.
  for (int count = 0; !Moving(*status) && count < 256; ++count) {
    auto event = motor_->GetNextEvent();
    if (!event) { FailDevice({axis_, "Poll", event.error().error_message()}); return; }
    if (!*event) break;
    stopped |= **event == thorlabs::MotorEvent::kStopped;
    completed |= (operation_ == OperationState::kHoming && **event == thorlabs::MotorEvent::kHomed) ||
                 ((operation_ == OperationState::kMoving || operation_ == OperationState::kJogging) &&
                  **event == thorlabs::MotorEvent::kMoveCompleted);
  }
  std::optional<int> done;
  std::optional<int> cancelled;
  std::optional<int> stop;
  if (stop_request_ && !Moving(*status) && (!active_request_ || stopped)) {
    stop = std::exchange(stop_request_, {});
    cancelled = std::exchange(active_request_, {});
  } else if (active_request_ && !Moving(*status) && (completed || stopped)) {
    if (stopped) cancelled = std::exchange(active_request_, {});
    else done = std::exchange(active_request_, {});
  }
  if (!active_request_ && !stop_request_) operation_ = OperationState::kIdle;
  AxisState observation{axis_, ConnectionState::kConnected, operation_};
  // Also show motion initiated outside this application (for example, the handset).
  if (operation_ == OperationState::kIdle) {
    if (status->homing) observation.operation = OperationState::kHoming;
    else if (status->jogging_forward || status->jogging_reverse)
      observation.operation = OperationState::kJogging;
    else if (status->moving_forward || status->moving_reverse)
      observation.operation = OperationState::kMoving;
  }
  observation.position_mm = *position;
  observation.homed = status->homed;
  observation.forward_limit = status->forward_limit_switch;
  observation.reverse_limit = status->reverse_limit_switch;
  emit StateChanged(observation);
  if (cancelled) emit RequestCancelled(*cancelled);
  if (done) emit RequestCompleted(*done);
  if (stop) emit RequestCompleted(*stop);
}
void MotorWorker::Shutdown() {
  shutting_down_ = true;
  poll_timer_->stop();
  motor_.reset();  // Wrapper stops, disables and closes the device.
  active_request_.reset();
  stop_request_.reset();
  emit ShutdownReady(axis_);
}
}  // namespace application
