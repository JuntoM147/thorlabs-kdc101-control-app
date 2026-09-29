#include "scan_controller.h"

#include <climits>
#include <cmath>
#include <exception>
#include <iomanip>
#include <sstream>
#include <utility>

#include "../../algo/algorithm/algorithm.h"

namespace application {
ScanController::ScanController(MotorController& x, MotorController& y, MotorController& z,
                               LaserController& laser, QObject* parent)
    : QObject(parent), motors_{x, y, z}, laser_(laser) {
  exposure_timer_ = new QTimer(this);
  exposure_timer_->setSingleShot(true);
  operation_timer_ = new QTimer(this);
  operation_timer_->setSingleShot(true);
  connect(exposure_timer_, &QTimer::timeout, this, &ScanController::OnExposureFinished);
  connect(operation_timer_, &QTimer::timeout, this, &ScanController::OnOperationTimedOut);
  for (auto& reference : motors_) {
    auto* motor = &reference.get();
    connect(motor, &MotorController::StateChanged, this, [this](AxisState state) {
      observations_[static_cast<std::size_t>(state.axis)] = state;
    });
    connect(motor, &MotorController::ScanOperationCompleted, this, &ScanController::OnMotorCompleted, Qt::QueuedConnection);
    connect(motor, &MotorController::ScanOperationFailed, this, &ScanController::OnMotorFailed, Qt::QueuedConnection);
    connect(motor, &MotorController::ScanOperationCancelled, this, &ScanController::OnMotorCancelled, Qt::QueuedConnection);
    connect(motor, &MotorController::RequestFailed, this, [this](int id, OperationError error) {
      if (id == 0 && state_.phase != ScanPhase::kIdle && state_.phase != ScanPhase::kWaiting &&
          state_.phase != ScanPhase::kFailed) FailScan(error);
    });
  }
  connect(&laser_, &LaserController::ScanOperationCompleted, this, &ScanController::OnLaserCompleted, Qt::QueuedConnection);
  connect(&laser_, &LaserController::ScanOperationFailed, this, &ScanController::OnLaserFailed, Qt::QueuedConnection);
  connect(&laser_, &LaserController::ScanOperationCancelled, this, &ScanController::OnLaserCancelled, Qt::QueuedConnection);
}
ScanController::~ScanController() = default;

std::expected<void, OperationError> ScanController::Configure(ScanConfiguration configuration) {
  auto invalid = [](const char* message) {
    return std::unexpected(OperationError{{}, "Configure scan", message});
  };
  if (state_.phase != ScanPhase::kIdle) return invalid("A scan is already configured or failed.");
  const auto& pattern = configuration.pattern;
  const auto start = configuration.start_pixel;
  const auto valid_setting = [](std::optional<double> value) {
    return !value || (std::isfinite(*value) && *value > 0);
  };
  if (pattern.Width() <= 0 || pattern.Height() <= 0 || pattern.PixelCount() <= 0 ||
      start.x < 0 || start.y < 0 || start.x >= pattern.Width() || start.y >= pattern.Height())
    return invalid("Provide a nonempty pattern and a start pixel inside it.");
  if (!std::isfinite(configuration.pixel_size_mm) || configuration.pixel_size_mm <= 0 ||
      !std::isfinite(configuration.pixel_size_mm * pattern.Width()) ||
      !std::isfinite(configuration.pixel_size_mm * pattern.Height()) ||
      configuration.exposure_time.count() < 0 || configuration.exposure_time.count() > INT_MAX ||
      configuration.motion_timeout.count() <= 0 || configuration.motion_timeout.count() > INT_MAX ||
      !valid_setting(configuration.motion.speed_mm_per_second) ||
      !valid_setting(configuration.motion.acceleration_mm_per_second_squared))
    return invalid("Invalid pixel size, duration or motion settings.");
  configuration_ = std::move(configuration);
  program_.clear();
  substep_ = 0;
  pending_devices_.clear();
  cleanup_started_ = cancelled_ = program_output_enabled_ = false;
  failure_error_.reset();
  state_ = {ScanPhase::kWaiting, 0, 0};
  emit StateChanged(state_);
  return {};
}
void ScanController::Start() {
  if (state_.phase != ScanPhase::kWaiting) {
    emit RequestFailed({{}, "Start scan", "No scan is waiting."}); return;
  }
  state_.phase = ScanPhase::kPreparing;
  substep_ = 0;
  emit StateChanged(state_);
  ContinueStopAndOff();
}
void ScanController::AwaitDevice(Device device) {
  if (pending_devices_.empty())
    operation_timer_->start(static_cast<int>(configuration_.motion_timeout.count()));
  pending_devices_.insert(device);
  const auto index = static_cast<std::size_t>(device);
  pending_distances_[index].reset();
  pending_start_positions_[index] = device == Device::kLaser ? std::nullopt
      : observations_[index].position_mm;
}
void ScanController::CancelPendingOperations() {
  const auto pending = pending_devices_;
  for (const auto device : pending) {
    if (device == Device::kLaser) laser_.CancelScanOperation();
    else motors_[static_cast<int>(device)].get().CancelScanOperation();
  }
}
void ScanController::ContinueStopAndOff() {
  if (state_.phase != ScanPhase::kPreparing && state_.phase != ScanPhase::kStopping) return;
  if (!pending_devices_.empty()) return;
  // Configure X and Y once, after all motors have stopped, before any scan move.
  if (state_.phase == ScanPhase::kPreparing && substep_ >= 4 && substep_ < 6) {
    const auto axis = substep_ - 4;
    AwaitDevice(static_cast<Device>(axis));
    motors_[axis].get().ConfigureForScan(configuration_.motion);
    return;
  }
  if (substep_ == (state_.phase == ScanPhase::kPreparing ? 6 : 4)) {
    if (state_.phase == ScanPhase::kPreparing) {
      // Confirm OFF/stopped before potentially expensive instruction generation.
      try {
        program_ = algo::GenerateInstructions(configuration_.start_pixel, configuration_.pattern);
      } catch (const std::exception& error) {
        FailScan({{}, "Generate scan", error.what()}); return;
      }
      state_.total_instructions = program_.size();
      substep_ = 0;
      state_.phase = ScanPhase::kRunning;
      emit StateChanged(state_);
      QTimer::singleShot(0, this, &ScanController::ExecuteNextInstruction);
    } else {
      FinishScan();
    }
    return;
  }
  // OFF first, then confirm X/Y/Z stopped. A failed cleanup still attempts all devices.
  if (substep_ == 0) {
    AwaitDevice(Device::kLaser);
    laser_.SetOutputForScan(false);
  } else {
    const auto axis = substep_ - 1;
    AwaitDevice(static_cast<Device>(axis));
    motors_[axis].get().StopForScan(StopMode::kImmediate);
  }
}
void ScanController::ExecuteNextInstruction() {
  if (!pending_devices_.empty() || exposure_timer_->isActive() ||
      (state_.phase != ScanPhase::kRunning && state_.phase != ScanPhase::kPauseRequested)) return;
  if (state_.completed_instructions == program_.size()) {
    StopMotorsAndTurnOutputOff(); return;
  }
  if (state_.phase == ScanPhase::kPauseRequested && substep_ == 0) {
    TurnOutputOffForPause(); return;
  }
  const auto& instruction = program_[state_.completed_instructions];
  if (const auto* move = std::get_if<algo::MoveRelative>(&instruction)) {
    if (program_output_enabled_ && (move->dx < 0 || move->dy < 0 ||
                                    (move->dx != 0 && move->dy != 0))) {
      FailScan({{}, "Exposure route", "Illuminated moves must use one positive axis. Reverse travel must be laser OFF."});
      return;
    }
    if (substep_ == 0) {
      substep_ = 1;
      // Reserve both axes before submitting either command. Terminal replies are
      // queued, including synchronous dispatch failures, so neither can advance
      // the instruction before all participants have been submitted.
      const std::array<int, 2> offsets{move->dx, move->dy};
      for (std::size_t axis = 0; axis < offsets.size(); ++axis) {
        if (offsets[axis] == 0) continue;
        AwaitDevice(static_cast<Device>(axis));
        pending_distances_[axis] = offsets[axis] * configuration_.pixel_size_mm;
      }
      for (std::size_t axis = 0; axis < offsets.size(); ++axis) {
        if (offsets[axis] != 0)
          motors_[axis].get().MoveRelativeForScan(*pending_distances_[axis]);
      }
      if (!pending_devices_.empty()) return;
    }
    FinishInstruction();
  } else if (substep_ != 0) {
    FinishInstruction();
  } else {
    substep_ = 1;
    const auto action = std::get<algo::Action>(instruction);
    if (action == algo::Action::kWait) {
      exposure_timer_->start(static_cast<int>(configuration_.exposure_time.count()));
    } else {
      program_output_enabled_ = action == algo::Action::kLaserOn;
      AwaitDevice(Device::kLaser);
      laser_.SetOutputForScan(program_output_enabled_);
    }
  }
}
void ScanController::FinishInstruction() {
  ++state_.completed_instructions;
  substep_ = 0;
  emit StateChanged(state_);
  QTimer::singleShot(0, this, &ScanController::ExecuteNextInstruction);
}
void ScanController::OnExposureFinished() {
  if (state_.phase == ScanPhase::kRunning || state_.phase == ScanPhase::kPauseRequested) FinishInstruction();
}
void ScanController::Pause() {
  if (state_.phase != ScanPhase::kRunning) {
    emit RequestFailed({{}, "Pause scan", "Scan is not running."}); return;
  }
  state_.phase = ScanPhase::kPauseRequested;
  emit StateChanged(state_);
  if (pending_devices_.empty() && !exposure_timer_->isActive())
    QTimer::singleShot(0, this, &ScanController::ExecuteNextInstruction);
}
void ScanController::TurnOutputOffForPause() {
  state_.phase = ScanPhase::kPausing;
  emit StateChanged(state_);
  if (state_.phase != ScanPhase::kPausing) return;
  AwaitDevice(Device::kLaser);
  laser_.SetOutputForScan(false);
}
void ScanController::Resume() {
  if (state_.phase != ScanPhase::kPaused) {
    emit RequestFailed({{}, "Resume scan", "Scan is not paused."}); return;
  }
  state_.phase = ScanPhase::kResuming;
  emit StateChanged(state_);
  if (state_.phase != ScanPhase::kResuming) return;
  AwaitDevice(Device::kLaser);
  laser_.SetOutputForScan(program_output_enabled_);
}
void ScanController::Cancel() {
  if (state_.phase == ScanPhase::kStopping) return;
  if (state_.phase == ScanPhase::kIdle || state_.phase == ScanPhase::kFailed) {
    emit RequestFailed({{}, "Cancel scan", "No active scan."}); return;
  }
  if (state_.phase == ScanPhase::kWaiting) {
    configuration_ = {};
    state_ = {};
    emit StateChanged(state_);
    emit ScanCancelled();
    return;
  }
  cancelled_ = true;
  state_.phase = ScanPhase::kStopping;
  state_.completed_instructions = 0;
  state_.total_instructions = 0;
  emit StateChanged(state_);
  exposure_timer_->stop();
  if (!pending_devices_.empty()) {
    CancelPendingOperations();
  } else {
    StopMotorsAndTurnOutputOff();
  }
}
void ScanController::StopMotorsAndTurnOutputOff() {
  if (!pending_devices_.empty()) return;
  exposure_timer_->stop();
  operation_timer_->stop();
  cleanup_started_ = true;
  state_.phase = ScanPhase::kStopping;
  substep_ = 0;
  emit StateChanged(state_);
  ContinueStopAndOff();
}
void ScanController::Reset() {
  if (!CanReset()) {
    emit RequestFailed({{}, "Reset scan", "Wait until the scan is paused, stopped or failed."});
    return;
  }
  failure_error_.reset();
  resetting_ = true;
  cancelled_ = false;
  program_output_enabled_ = false;
  configuration_ = {};
  configuration_.motion_timeout = std::chrono::seconds(10);
  program_.clear();
  state_.completed_instructions = state_.total_instructions = 0;
  StopMotorsAndTurnOutputOff();
}
void ScanController::FailScan(OperationError error) {
  if (!failure_error_) failure_error_ = std::move(error);
  if (state_.phase == ScanPhase::kStopping && cleanup_started_) return;
  state_.phase = ScanPhase::kStopping;
  emit StateChanged(state_);
  exposure_timer_->stop();
  if (!pending_devices_.empty()) {
    CancelPendingOperations();
  } else {
    StopMotorsAndTurnOutputOff();
  }
}
void ScanController::OperationCompleted(Device device) {
  if (!pending_devices_.erase(device)) return;
  if (!pending_devices_.empty()) return;
  operation_timer_->stop();
  if (state_.phase == ScanPhase::kPreparing ||
      (state_.phase == ScanPhase::kStopping && cleanup_started_)) {
    ++substep_;
    QTimer::singleShot(0, this, &ScanController::ContinueStopAndOff);
  } else if (state_.phase == ScanPhase::kStopping) {
    StopMotorsAndTurnOutputOff();
  } else if (state_.phase == ScanPhase::kPausing) {
    state_.phase = ScanPhase::kPaused;
    emit StateChanged(state_);
  } else if (state_.phase == ScanPhase::kResuming) {
    state_.phase = ScanPhase::kRunning;
    emit StateChanged(state_);
    QTimer::singleShot(0, this, &ScanController::ExecuteNextInstruction);
  } else {
    QTimer::singleShot(0, this, &ScanController::ExecuteNextInstruction);
  }
}
void ScanController::OperationFailed(Device device, OperationError error) {
  if (!pending_devices_.erase(device)) return;
  if (pending_devices_.empty()) operation_timer_->stop();
  if (!failure_error_) failure_error_ = error;
  if (cleanup_started_ && state_.phase == ScanPhase::kStopping) {
    ++substep_;
    QTimer::singleShot(0, this, &ScanController::ContinueStopAndOff);
  } else {
    FailScan(error);
  }
}
void ScanController::OperationCancelled(Device device) {
  if (!pending_devices_.contains(device)) return;
  if (state_.phase == ScanPhase::kStopping) OperationCompleted(device);
  else OperationFailed(device, {{}, "Scan", "Device operation was interrupted."});
}
void ScanController::OnMotorCompleted(Axis axis) { OperationCompleted(static_cast<Device>(axis)); }
void ScanController::OnMotorFailed(Axis axis, OperationError error) { OperationFailed(static_cast<Device>(axis), error); }
void ScanController::OnMotorCancelled(Axis axis) { OperationCancelled(static_cast<Device>(axis)); }
void ScanController::OnLaserCompleted() { OperationCompleted(Device::kLaser); }
void ScanController::OnLaserFailed(OperationError error) { OperationFailed(Device::kLaser, error); }
void ScanController::OnLaserCancelled() { OperationCancelled(Device::kLaser); }
void ScanController::OnOperationTimedOut() {
  if (pending_devices_.empty()) return;
  if (!failure_error_) {
    std::ostringstream message;
    message << std::fixed << std::setprecision(6)
            << "No completion confirmation within " << configuration_.motion_timeout.count() / 1000.0
            << " seconds.";
    std::optional<Axis> axis;
    for (const auto device : pending_devices_) {
      if (device == Device::kLaser) {
        message << " Waiting for laser output acknowledgement.";
        continue;
      }
      const auto index = static_cast<std::size_t>(device);
      if (pending_devices_.size() == 1) axis = static_cast<Axis>(device);
      const auto& observation = observations_[index];
      message << " Axis " << "XYZ"[index] << ':';
      if (pending_distances_[index]) message << " Requested relative move: " << *pending_distances_[index] << " mm.";
      if (pending_start_positions_[index]) message << " Start: " << *pending_start_positions_[index] << " mm.";
      if (observation.position_mm) message << " Last position: " << *observation.position_mm << " mm.";
      const auto flag = [](std::optional<bool> value) {
        return value ? (*value ? "yes" : "no") : "unknown";
      };
      message << " Hardware moving=" << flag(observation.hardware_moving)
              << ", homed=" << flag(observation.homed)
              << ", enabled=" << flag(observation.channel_enabled)
              << ", forward limit=" << flag(observation.forward_limit)
              << ", reverse limit=" << flag(observation.reverse_limit) << ".";
    }
    failure_error_ = OperationError{axis, "Scan timeout", message.str()};
  }
  // Retain all device reservations until their cancellation replies settle.
  if (state_.phase != ScanPhase::kStopping || !cleanup_started_) FailScan(*failure_error_);
  else CancelPendingOperations();
}
void ScanController::FinishScan() {
  operation_timer_->stop();
  exposure_timer_->stop();
  configuration_ = {};
  program_.clear();
  state_.phase = failure_error_ ? ScanPhase::kFailed : ScanPhase::kIdle;
  const bool reset = std::exchange(resetting_, false);
  if (reset && failure_error_) {
    failure_error_->operation = "Reset scan / " + failure_error_->operation;
    failure_error_->message += " Reset could not confirm laser OFF and all motors stopped. Manual controls remain locked.";
  }
  emit StateChanged(state_);
  if (failure_error_) emit ScanFailed(*failure_error_);
  else if (reset) emit ResetCompleted();
  else if (cancelled_) emit ScanCancelled();
  else emit ScanCompleted();
}
}  // namespace application
