#include "laser_worker.h"

namespace application {
LaserWorker::LaserWorker(QObject* parent) : QObject(parent) {}
LaserWorker::~LaserWorker() = default;
void LaserWorker::Connect(int id, LaserConnection connection) {
  if (shutting_down_ || laser_ || connection.digital_output_channel.empty()) {
    emit RequestFailed(id, {{}, "Connect laser", "Invalid channel or laser already connected."}); return;
  }
  state_ = {ConnectionState::kConnecting, {}};
  emit StateChanged(state_);
  auto result = NI_DAQ::Laser::CreateLaser(connection.digital_output_channel);
  if (!result) {
    state_ = {};
    emit StateChanged(state_);
    emit RequestFailed(id, {{}, "Connect laser", result.error().error_message()});
    return;
  }
  laser_ = std::move(*result);
  state_ = {ConnectionState::kConnected, false};  // CreateLaser confirms an OFF write.
  emit StateChanged(state_);
  emit RequestCompleted(id);
}
void LaserWorker::SetOutputEnabled(int id, bool enabled) {
  if (!laser_ || shutting_down_) {
    emit RequestFailed(id, {{}, "Laser output", "Laser is disconnected."}); return;
  }
  auto result = enabled ? laser_->TurnOn() : laser_->TurnOff();
  state_.output_enabled = result.ok() ? std::optional<bool>(enabled) : std::nullopt;
  emit StateChanged(state_);
  if (result.ok()) emit RequestCompleted(id);
  else emit RequestFailed(id, {{}, "Laser output", result.error_message()});
}
void LaserWorker::Disconnect(int id) {
  if (laser_) {
    auto result = laser_->TurnOff();
    if (!result.ok()) {
      state_.output_enabled.reset();
      emit StateChanged(state_);
      emit RequestFailed(id, {{}, "Disconnect laser", result.error_message()});
      return;
    }
  }
  laser_.reset();
  state_ = {};
  emit StateChanged(state_);
  emit RequestCompleted(id);
}
void LaserWorker::Shutdown() {
  shutting_down_ = true;
  laser_.reset();  // Wrapper attempts OFF before clearing its task.
  state_ = {};
  emit ShutdownReady();
}
}  // namespace application