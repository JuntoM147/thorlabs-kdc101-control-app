#include "laser_worker.h"

#include <utility>

#include "devices/ni_daq_laser/ni_daq_laser.h"

namespace workers {

LaserWorker::LaserWorker(QObject* parent) : QObject(parent) {}
LaserWorker::~LaserWorker() = default;

void LaserWorker::FinishRequest(RequestId id, errors::Error result) {
  // Publish observations before completion so the UI sees the confirmed state
  // before enabling another command.
  emit StateChanged(state_);
  emit RequestFinished(id, result);
}

bool LaserWorker::RejectDuringShutdown(RequestId id) {
  if (!shutting_down_) return false;
  FinishRequest(id, errors::Error::Failure(errors::ErrorCode::kControlDenied,
                                           "Laser worker is shutting down."));
  return true;
}

void LaserWorker::ConnectDevice(RequestId id, QString output_channel) {
  if (RejectDuringShutdown(id)) return;
  if (laser_) {
    FinishRequest(id, errors::Error::Failure(
                          errors::ErrorCode::kBusy,
                          "Disconnect the laser before reconnecting."));
    return;
  }
  output_channel = output_channel.trimmed();
  if (output_channel.isEmpty()) {
    FinishRequest(
        id, errors::Error::InvalidArgument("Enter a digital output channel."));
    return;
  }

  auto result = NI_DAQ::Laser::CreateLaser(output_channel.toStdString());
  if (!result) {
    FinishRequest(id, result.error().WithContext("Connect laser"));
    return;
  }
  laser_ = std::move(*result);
  // CreateLaser succeeds only after its initial laser-off write succeeds.
  state_ = {true, false};
  FinishRequest(id, errors::Error::Ok());
}

void LaserWorker::SetOutput(RequestId id, bool enabled) {
  if (RejectDuringShutdown(id)) return;
  if (!laser_) {
    FinishRequest(id, errors::Error::Failure(
                          errors::ErrorCode::kConnectionError,
                          "Connect the laser before setting its output."));
    return;
  }

  auto result = enabled ? laser_->TurnOn() : laser_->TurnOff();
  if (result.ok())
    state_.output_enabled = enabled;
  else
    state_.output_enabled.reset();  // A failed write cannot confirm the output.
  FinishRequest(id, result.WithContext("Set laser output"));
}

void LaserWorker::DisconnectDevice(RequestId id) {
  if (RejectDuringShutdown(id)) return;
  if (laser_) {
    auto result = laser_->TurnOff();
    if (!result.ok()) {
      // Keep the device open so an explicit off/disconnect retry is possible.
      state_.output_enabled.reset();
      FinishRequest(id,
                    result.WithContext("Turn laser off before disconnecting"));
      return;
    }
    laser_.reset();
  }
  state_ = {};
  FinishRequest(id, errors::Error::Ok());
}

void LaserWorker::Shutdown() {
  if (shutting_down_) return;
  shutting_down_ = true;
  if (laser_) {
    auto result = laser_->TurnOff();
    if (!result.ok())
      emit ShutdownFailed(result.WithContext("Turn laser off during shutdown"));
    // The wrapper destructor also attempts off before releasing the DAQ task.
    laser_.reset();
  }
  // Disconnected output state is unknown, even after successful off.
  state_ = {};
  emit StateChanged(state_);
  emit ShutdownFinished();
}

}  // namespace workers
