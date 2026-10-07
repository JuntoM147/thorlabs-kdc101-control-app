#include "scan_worker.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace workers {
ScanWorker::ScanWorker(QObject* parent) : QObject(parent) {}
ScanWorker::~ScanWorker() = default;

void ScanWorker::EnsureTimers() {
  if (operation_timer_) return;
  operation_timer_ = new QTimer(this);
  operation_timer_->setSingleShot(true);
  connect(operation_timer_, &QTimer::timeout, this, &ScanWorker::OnTimeout);
  exposure_timer_ = new QTimer(this);
  exposure_timer_->setSingleShot(true);
  connect(exposure_timer_, &QTimer::timeout, this,
          &ScanWorker::OnExposureFinished);
  progress_timer_ = new QTimer(this);
  progress_timer_->setInterval(1000);
  connect(progress_timer_, &QTimer::timeout, this,
          &ScanWorker::PublishProgress);
}

void ScanWorker::Start(ScanJob job) {
  if (shutdown_requested_ || progress_.phase != ScanPhase::kIdle) return;
  EnsureTimers();
  const auto max_interval = std::numeric_limits<int>::max();
  if (job.pixel_exposure.count() <= 0 ||
      job.pixel_exposure.count() > max_interval ||
      job.operation_timeout.count() <= 0 ||
      job.operation_timeout.count() > max_interval ||
      job.motion_timeout.count() <= 0 ||
      job.motion_timeout.count() > max_interval) {
    emit Finished(errors::Error::InvalidArgument(
        "Scan timer intervals must be positive and supported."));
    return;
  }
  // Validate the program before any motor or laser requests are emitted.
  for (const auto& instruction : job.instructions) {
    if (const auto* move = std::get_if<algo::MoveAbsolute>(&instruction)) {
      if (!std::isfinite(move->x_mm) || !std::isfinite(move->y_mm)) {
        emit Finished(errors::Error::InvalidArgument(
            "Scan move exceeds the supported distance."));
        return;
      }
    } else {
      const auto action = std::get<algo::Action>(instruction);
      if (action != algo::Action::kLaserOn &&
          action != algo::Action::kLaserOff && action != algo::Action::kWait) {
        emit Finished(errors::Error::InvalidArgument("Unknown scan action."));
        return;
      }
    }
  }
  job_ = std::move(job);
  progress_ = {ScanPhase::kRunning, 0, job_.instructions.size()};
  active_elapsed_ms_ = 0;
  active_timer_.start();
  progress_timer_->start();
  pause_requested_ = false;
  program_laser_on_ = false;
  restoring_laser_ = false;
  pause_laser_off_ = false;
  final_result_.reset();
  PublishProgress();
  capturing_origin_ = true;
  pending_x_ = NextRequestId();
  pending_y_ = NextRequestId();
  operation_timer_->start(static_cast<int>(job_.operation_timeout.count()));
  emit PositionRequested(Axis::kX, *pending_x_);
  emit PositionRequested(Axis::kY, *pending_y_);
}

RequestId ScanWorker::NextRequestId() { return next_request_id_++; }
void ScanWorker::OnPositionCaptured(Axis axis, RequestId id, double position_mm) {
  if (!capturing_origin_ || progress_.phase != ScanPhase::kRunning) return;
  if (axis == Axis::kX && pending_x_ == id) origin_x_mm_ = position_mm;
  if (axis == Axis::kY && pending_y_ == id) origin_y_mm_ = position_mm;
}
void ScanWorker::PublishProgress() {
  const bool active = progress_.phase == ScanPhase::kRunning ||
                      progress_.phase == ScanPhase::kPausing;
  if (active || progress_.phase == ScanPhase::kPaused) {
    const auto elapsed_ms =
        active_elapsed_ms_ +
        (active_timer_.isValid() ? active_timer_.elapsed() : 0);
    const auto completed = progress_.completed_instructions;
    // Wait for a small sample before extrapolating. Instructions vary in cost,
    // so this is deliberately a rough estimate rather than a motion model.
    if (completed >= 3 && elapsed_ms >= 1000) {
      const auto remaining = progress_.total_instructions - completed;
      const long double seconds =
          std::ceil((elapsed_ms / 1000.0L) * remaining / completed);
      progress_.estimated_remaining = std::chrono::seconds(static_cast<qint64>(
          std::min(seconds, static_cast<long double>(
                                std::numeric_limits<qint64>::max()))));
    }
  } else {
    progress_.estimated_remaining.reset();
  }
  emit ProgressChanged(progress_);
}

void ScanWorker::RequestLaser(bool enabled) {
  pending_laser_ = NextRequestId();
  operation_timer_->start(static_cast<int>(job_.operation_timeout.count()));
  emit LaserOutputRequested(*pending_laser_, enabled);
}

void ScanWorker::ExecuteNextInstruction() {
  if (progress_.phase != ScanPhase::kRunning) return;
  if (pending_x_ || pending_y_ || pending_laser_ || exposure_timer_->isActive())
    return;
  if (progress_.completed_instructions == job_.instructions.size()) {
    BeginCleanup(errors::Error::Ok());
    return;
  }
  if (pause_requested_) {
    progress_.phase = ScanPhase::kPausing;
    pause_laser_off_ = true;
    PublishProgress();
    RequestLaser(false);
    return;
  }
  const auto& instruction = job_.instructions[progress_.completed_instructions];
  if (const auto* move = std::get_if<algo::MoveAbsolute>(&instruction)) {
    const double x = origin_x_mm_ + move->x_mm;
    const double y = origin_y_mm_ + move->y_mm;
    if (!std::isfinite(x) || !std::isfinite(y)) {
      BeginCleanup(errors::Error::InvalidArgument("Scan target is not finite."));
      return;
    }
    // Allocate both IDs before emitting; diagonal moves wait for both axes.
    pending_x_ = NextRequestId();
    pending_y_ = NextRequestId();
    operation_timer_->start(static_cast<int>(job_.motion_timeout.count()));
    if (pending_x_)
      emit MoveRequested(Axis::kX, *pending_x_, x);
    if (pending_y_)
      emit MoveRequested(Axis::kY, *pending_y_, y);
    return;
  }
  switch (std::get<algo::Action>(instruction)) {
    case algo::Action::kLaserOn:
      RequestLaser(true);
      break;
    case algo::Action::kLaserOff:
      RequestLaser(false);
      break;
    case algo::Action::kWait:
      exposure_timer_->start(static_cast<int>(job_.pixel_exposure.count()));
      break;
  }
}

void ScanWorker::CompleteInstruction() {
  operation_timer_->stop();
  ++progress_.completed_instructions;
  PublishProgress();
  // Yield between instructions so queued pause/cancel requests can run.
  QMetaObject::invokeMethod(this, &ScanWorker::ExecuteNextInstruction,
                            Qt::QueuedConnection);
}
void ScanWorker::OnExposureFinished() {
  if (progress_.phase == ScanPhase::kRunning) CompleteInstruction();
}

void ScanWorker::OnMotorFinished(Axis axis, RequestId id,
                                 errors::Error result) {
  std::optional<RequestId>* pending = nullptr;
  if (axis == Axis::kX) pending = &pending_x_;
  if (axis == Axis::kY) pending = &pending_y_;
  if (!pending || *pending != id) return;
  pending->reset();
  if (!result.ok()) {
    if (progress_.phase != ScanPhase::kStopping) {
      BeginCleanup(result);
      return;
    }
    cleanup_failed_ = true;
    final_result_ = result.WithContext("Stop motor during scan cleanup");
  }
  CheckPendingRequests();
}

void ScanWorker::OnLaserFinished(RequestId id, errors::Error result) {
  if (pending_laser_ != id) return;
  pending_laser_.reset();
  if (!result.ok()) {
    if (progress_.phase != ScanPhase::kStopping) {
      BeginCleanup(result);
      return;
    }
    cleanup_failed_ = true;
    final_result_ = result.WithContext("Turn laser off during scan cleanup");
  }
  if (progress_.phase == ScanPhase::kStopping) {
    CheckPendingRequests();
    return;
  }
  operation_timer_->stop();
  if (pause_laser_off_) {
    pause_laser_off_ = false;
    progress_.phase = ScanPhase::kPaused;
    active_elapsed_ms_ += active_timer_.elapsed();
    active_timer_.invalidate();
    progress_timer_->stop();
    PublishProgress();
  } else if (restoring_laser_) {
    restoring_laser_ = false;
    QMetaObject::invokeMethod(this, &ScanWorker::ExecuteNextInstruction,
                              Qt::QueuedConnection);
  } else {
    program_laser_on_ =
        std::get<algo::Action>(
            job_.instructions[progress_.completed_instructions]) ==
        algo::Action::kLaserOn;
    CompleteInstruction();
  }
}

void ScanWorker::CheckPendingRequests() {
  if (pending_x_ || pending_y_ || pending_laser_) return;
  if (progress_.phase == ScanPhase::kStopping)
    FinishCleanup();
  else if (capturing_origin_) {
    capturing_origin_ = false;
    operation_timer_->stop();
    ExecuteNextInstruction();
  } else
    CompleteInstruction();
}

void ScanWorker::Pause() {
  if (progress_.phase == ScanPhase::kRunning) pause_requested_ = true;
}
void ScanWorker::OnMotorDisconnected(Axis axis) {
  if (progress_.phase != ScanPhase::kRunning &&
      progress_.phase != ScanPhase::kPausing &&
      progress_.phase != ScanPhase::kPaused)
    return;
  BeginCleanup(errors::Error::Failure(
      errors::ErrorCode::kConnectionError,
      "Axis " + std::string(1, "XYZ"[static_cast<int>(axis)]) +
          " disconnected during the scan."));
}
void ScanWorker::Resume() {
  if (progress_.phase != ScanPhase::kPaused) return;
  pause_requested_ = false;
  progress_.phase = ScanPhase::kRunning;
  active_timer_.start();
  progress_timer_->start();
  PublishProgress();
  if (program_laser_on_) {
    restoring_laser_ = true;
    RequestLaser(true);
  } else {
    QMetaObject::invokeMethod(this, &ScanWorker::ExecuteNextInstruction,
                              Qt::QueuedConnection);
  }
}

void ScanWorker::BeginCleanup(errors::Error result) {
  capturing_origin_ = false;
  EnsureTimers();
  operation_timer_->stop();
  exposure_timer_->stop();
  progress_timer_->stop();
  active_timer_.invalidate();
  progress_.phase = ScanPhase::kStopping;
  final_result_ = result;
  cleanup_failed_ = false;
  pause_laser_off_ = restoring_laser_ = false;
  // Replace in-flight IDs. Late completions from the interrupted instruction
  // are ignored; cleanup waits for these three new acknowledgements instead.
  pending_x_ = NextRequestId();
  pending_y_ = NextRequestId();
  pending_laser_ = NextRequestId();
  PublishProgress();
  operation_timer_->start(static_cast<int>(job_.operation_timeout.count()));
  emit LaserOutputRequested(*pending_laser_, false);
  emit StopRequested(Axis::kX, *pending_x_, thorlabs::StopMode::kImmediate);
  emit StopRequested(Axis::kY, *pending_y_, thorlabs::StopMode::kImmediate);
}

void ScanWorker::FinishCleanup() {
  operation_timer_->stop();
  pending_x_.reset();
  pending_y_.reset();
  pending_laser_.reset();
  program_laser_on_ = false;
  progress_.phase = cleanup_failed_ ? ScanPhase::kFailed : ScanPhase::kIdle;
  PublishProgress();
  const auto result = *final_result_;
  final_result_.reset();
  emit Finished(result);
  if (shutdown_requested_) emit ShutdownFinished();
}

void ScanWorker::Cancel() {
  if (progress_.phase == ScanPhase::kStopping) return;
  BeginCleanup(
      errors::Error::Failure(errors::ErrorCode::kCancelled, "Scan cancelled."));
}
void ScanWorker::Shutdown() {
  if (shutdown_requested_) return;
  shutdown_requested_ = true;
  if (progress_.phase == ScanPhase::kStopping) return;
  BeginCleanup(errors::Error::Failure(errors::ErrorCode::kCancelled,
                                      "Scan worker shut down."));
}
void ScanWorker::OnTimeout() {
  const auto error = errors::Error::Failure(errors::ErrorCode::kTimeout,
                                            "Scan operation timed out.");
  if (progress_.phase == ScanPhase::kStopping) {
    cleanup_failed_ = true;
    final_result_ = error.WithContext("Scan cleanup");
    FinishCleanup();
  } else {
    BeginCleanup(error);
  }
}
}  // namespace workers
