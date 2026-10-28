#include "nrt100/nrt100.h"

#include <cmath>
#include <expected>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#include "Thorlabs.MotionControl.Benchtop.StepperMotor.h"
#include "error/error.h"

namespace thorlabs {
using errors::Error;
namespace {
using Direction = NRT100::Direction;
using StopMode = NRT100::StopMode;
using JogMode = NRT100::JogMode;
using VelocityParameters = NRT100::VelocityParameters;
using MotorConfiguration = NRT100::MotorConfiguration;
using MotorStatus = NRT100::MotorStatus;
using MotorEvent = NRT100::MotorEvent;
}  // namespace

namespace {

// Acquisition and release are serialized across axis worker threads. Keeping
// channel ownership until cleanup completes prevents a reconnect racing
// teardown.
std::mutex controller_mutex;
std::map<std::string, std::set<short>> controller_channels;

Error OpenDevice(const std::string& serial_number, short channel) {
  std::lock_guard lock(controller_mutex);
  auto existing = controller_channels.find(serial_number);
  if (existing != controller_channels.end()) {
    if (!existing->second.insert(channel).second)
      return Error::InvalidArgument(
          "Controller channel already belongs to another axis.");
    return Error::Ok();
  }
  auto list_status = Error::FromKinesis(TLI_BuildDeviceList());
  if (!list_status.ok()) return list_status.WithContext("TLI_BuildDeviceList");
  Error open_status = Error::FromKinesis(SBC_Open(serial_number.c_str()));
  if (!open_status.ok()) {
    return open_status.WithContext("SBC_Open");
  }
  controller_channels[serial_number].insert(channel);

  return Error::Ok();
}

Error LoadSettings(const std::string& serial_number, short channel,
                   NRT100::StageModel stage) {
  const char* profile = stage == NRT100::StageModel::kNrt150M
                            ? "HS NRT150/M Enc Stage 150mm"
                            : "HS NRT100/M Enc Stage 100mm";
  // Use the known actuator profile, not a previously saved stage selection.
  if (!SBC_LoadNamedSettings(serial_number.c_str(), channel, profile)) {
    return Error::FailedToLoadSettings(serial_number)
        .WithContext(std::string("SBC_LoadNamedSettings: ") + profile);
  }

  return Error::Ok();
}

Error EnableChannel(const std::string& serial_number, short channel) {
  Error channel_status =
      Error::FromKinesis(SBC_EnableChannel(serial_number.c_str(), channel));
  if (!channel_status.ok()) {
    return channel_status.WithContext("SBC_EnableChannel");
  }

  return Error::Ok();
}

Error StartPolling(const std::string& serial_number, short channel,
                   int polling_interval_ms) {
  // Start the device polling
  if (!SBC_StartPolling(serial_number.c_str(), channel, polling_interval_ms)) {
    return Error::FailedToStartPolling(serial_number)
        .WithContext("SBC_StartPolling");
  }

  return Error::Ok();
}

}  // namespace

NRT100::NRT100(std::string serial_number, short channel,
               std::shared_ptr<const KinesisSimulation> simulation,
               StageModel stage)
    : serial_number_{serial_number},
      channel_{channel},
      stage_{stage},
      simulation_{std::move(simulation)},
      connected_{false},
      polling_{false},
      channel_enabled_{false} {}

NRT100::CreateResult NRT100::CreateMotor(
    std::string serial_number, short channel, int polling_interval_ms,
    std::shared_ptr<const KinesisSimulation> simulation, StageModel stage) {
  auto device = std::unique_ptr<NRT100>(
      new NRT100(serial_number, channel, std::move(simulation), stage));

  if (stage != StageModel::kNrt100M && stage != StageModel::kNrt150M) {
    return std::unexpected(Error::InvalidArgument("Unknown NRT stage model."));
  }
  if (serial_number.empty() || channel < 1 || polling_interval_ms <= 0) {
    return std::unexpected(Error::InvalidArgument(
        "Serial, channel and polling interval must be valid."));
  }
  auto status = OpenDevice(serial_number, channel);
  if (!status.ok()) return std::unexpected(status);
  device->connected_ = true;
  if (channel > SBC_GetNumChannels(serial_number.c_str())) {
    return std::unexpected(
        Error::InvalidArgument("Controller channel does not exist."));
  }

  // Load Settings
  status = LoadSettings(serial_number, channel, stage);
  if (!status.ok()) {
    return std::unexpected(status);
  }

  // Enable Channel
  status = EnableChannel(serial_number, channel);
  if (!status.ok()) {
    return std::unexpected(status);
  }

  device->channel_enabled_ = true;

  // Start Polling
  status = StartPolling(serial_number, channel, polling_interval_ms);
  if (!status.ok()) {
    return std::unexpected(status);
  }

  device->polling_ = true;

  // Keep the stage-specific velocity, home and jog settings from Kinesis.
  return device;
}

NRT100::~NRT100() {
  std::lock_guard lock(controller_mutex);
  if (channel_enabled_) {
    SBC_ClearMessageQueue(serial_number_.c_str(), channel_);
    SBC_StopProfiled(serial_number_.c_str(),
                     channel_);  // No point waiting for stop to finish
  }

  if (channel_enabled_) {
    SBC_DisableChannel(serial_number_.c_str(), channel_);
  }

  if (polling_) {
    SBC_StopPolling(serial_number_.c_str(), channel_);
  }

  if (connected_) {
    auto owner = controller_channels.find(serial_number_);
    if (owner != controller_channels.end()) {
      owner->second.erase(channel_);
      if (owner->second.empty()) {
        SBC_Close(serial_number_.c_str());
        controller_channels.erase(owner);
      }
    }
  }
}

namespace {

constexpr int kUnitTypeDistance = 0;
constexpr int kUnitTypeSpeed = 1;
constexpr int kUnitTypeAcceleration = 2;

std::expected<int, Error> ConvertMotionParameter(
    const std::string& serial_number, short channel, double value,
    int unit_type) {
  if (!std::isfinite(value) || value <= 0.0) {
    return std::unexpected(Error::FromKinesis(FT_InvalidParameter));
  }
  int device_units = 0;
  auto status = Error::FromKinesis(SBC_GetDeviceUnitFromRealValue(
      serial_number.c_str(), channel, value, &device_units, unit_type));

  if (!status.ok()) {
    return std::unexpected(status);
  }

  if (device_units <= 0) {
    return std::unexpected(Error::FromKinesis(FT_InvalidParameter));
  }

  return device_units;
}

Error ApplyVelocity(const std::string& serial_number, short channel,
                    VelocityParameters update, bool jog) {
  std::optional<int> speed, acceleration;
  if (update.speed_mm_per_second) {
    auto converted = ConvertMotionParameter(
        serial_number, channel, *update.speed_mm_per_second, kUnitTypeSpeed);
    if (!converted) return converted.error();
    speed = *converted;
  }
  if (update.acceleration_mm_per_second_squared) {
    auto converted = ConvertMotionParameter(
        serial_number, channel, *update.acceleration_mm_per_second_squared,
        kUnitTypeAcceleration);
    if (!converted) return converted.error();
    acceleration = *converted;
  }
  if (!speed && !acceleration) return Error::Ok();
  int device_speed = 0, device_acceleration = 0;
  const char* serial = serial_number.c_str();
  auto status = Error::FromKinesis(
      jog ? SBC_GetJogVelParams(serial, channel, &device_acceleration,
                                &device_speed)
          : SBC_GetVelParams(serial, channel, &device_acceleration,
                             &device_speed));
  if (!status.ok()) return status;
  // Explicit updates must reach the controller even if the SDK's cached
  // parameters already match. Preserve only fields omitted by the caller.
  return Error::FromKinesis(
      jog ? SBC_SetJogVelParams(serial, channel,
                                acceleration.value_or(device_acceleration),
                                speed.value_or(device_speed))
          : SBC_SetVelParams(serial, channel,
                             acceleration.value_or(device_acceleration),
                             speed.value_or(device_speed)));
}

}  // namespace

NRT100::PositionResult NRT100::GetPosition() {
  auto connection = CheckConnection();
  if (!connection.ok()) return std::unexpected(connection);
  int device_unit = SBC_GetPosition(serial_number_.c_str(), channel_);

  double position_real;
  Error conversion_status = Error::FromKinesis(SBC_GetRealValueFromDeviceUnit(
      serial_number_.c_str(), channel_, device_unit, &position_real,
      kUnitTypeDistance));

  if (!conversion_status.ok()) {
    return std::unexpected(conversion_status);
  }

  return position_real;
}

Error NRT100::SetMoveVelocity(VelocityParameters parameters) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  return ApplyVelocity(serial_number_, channel_, parameters, false);
}

std::expected<MotorConfiguration, Error> NRT100::GetConfiguration() {
  auto status = CheckConnection();
  if (!status.ok()) return std::unexpected(status);
  int acceleration = 0, speed = 0;
  status = Error::FromKinesis(SBC_GetVelParams(serial_number_.c_str(), channel_,
                                               &acceleration, &speed));
  if (!status.ok()) return std::unexpected(status);
  MotorConfiguration configuration{};
  const auto read = [this](int units, int type, double& value) {
    return Error::FromKinesis(SBC_GetRealValueFromDeviceUnit(
        serial_number_.c_str(), channel_, units, &value, type));
  };
  status = read(acceleration, kUnitTypeAcceleration,
                configuration.acceleration_mm_per_second_squared);
  if (!status.ok()) return std::unexpected(status);
  status = read(SBC_GetHomingVelocity(serial_number_.c_str(), channel_),
                kUnitTypeSpeed, configuration.homing_speed_mm_per_second);
  if (!status.ok()) return std::unexpected(status);
  status = read(SBC_GetBacklash(serial_number_.c_str(), channel_),
                kUnitTypeDistance, configuration.backlash_mm);
  if (!status.ok()) return std::unexpected(status);
  return configuration;
}

NRT100::PositionResult NRT100::GetDistanceResolution() {
  auto connection = CheckConnection();
  if (!connection.ok()) return std::unexpected(connection);
  double resolution = 0;
  const auto status = Error::FromKinesis(SBC_GetRealValueFromDeviceUnit(
      serial_number_.c_str(), channel_, 1, &resolution, kUnitTypeDistance));
  if (!status.ok()) {
    return std::unexpected(status);
  }

  if (!std::isfinite(resolution) || resolution <= 0) {
    return std::unexpected(Error::FromKinesis(FT_InvalidParameter));
  }

  return resolution;
}

Error NRT100::SetJogVelocity(VelocityParameters parameters) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  return ApplyVelocity(serial_number_, channel_, parameters, true);
}

Error NRT100::SetHomingSpeed(double speed) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  auto converted =
      ConvertMotionParameter(serial_number_, channel_, speed, kUnitTypeSpeed);
  if (!converted) {
    return converted.error();
  }

  return Error::FromKinesis(SBC_SetHomingVelocity(
      serial_number_.c_str(), channel_, static_cast<unsigned int>(*converted)));
}

Error NRT100::SetBacklash(double distance_mm) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  if (!std::isfinite(distance_mm) || distance_mm < 0.0) {
    return Error::InvalidArgument(
        "Backlash must be a finite, non-negative distance in mm.");
  }
  if (distance_mm == 0.0) {
    return Error::FromKinesis(
        SBC_SetBacklash(serial_number_.c_str(), channel_, 0));
  }

  auto converted = ConvertMotionParameter(serial_number_, channel_, distance_mm,
                                          kUnitTypeDistance);
  if (!converted) return converted.error();
  return Error::FromKinesis(
      SBC_SetBacklash(serial_number_.c_str(), channel_, *converted));
}

Error NRT100::SetJogStepSize(double step) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  auto converted =
      ConvertMotionParameter(serial_number_, channel_, step, kUnitTypeDistance);
  if (!converted) {
    return converted.error();
  }
  return Error::FromKinesis(SBC_SetJogStepSize(
      serial_number_.c_str(), channel_, static_cast<unsigned int>(*converted)));
}

Error NRT100::SetJogMode(JogMode mode, StopMode stop_mode) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  if ((mode != JogMode::kSingleStep && mode != JogMode::kContinuous) ||
      (stop_mode != StopMode::kProfiled && stop_mode != StopMode::kImmediate)) {
    return Error::FromKinesis(FT_InvalidParameter);
  }
  return Error::FromKinesis(SBC_SetJogMode(
      serial_number_.c_str(), channel_,
      mode == JogMode::kSingleStep ? MOT_JogModes::MOT_SingleStep
                                   : MOT_JogModes::MOT_Continuous,
      stop_mode == StopMode::kProfiled ? MOT_StopModes::MOT_Profiled
                                       : MOT_StopModes::MOT_Immediate));
}

Error NRT100::StartHome() {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  return Error::FromKinesis(SBC_Home(serial_number_.c_str(), channel_));
}

Error NRT100::StartJog(Direction dir) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  if (dir != Direction::kForward && dir != Direction::kBackward)
    return Error::FromKinesis(FT_InvalidParameter);
  MOT_TravelDirection mot_direction;

  if (dir == Direction::kForward) {
    mot_direction = MOT_TravelDirection::MOT_Forwards;
  } else {
    mot_direction = MOT_TravelDirection::MOT_Reverse;
  }

  return Error::FromKinesis(
      SBC_MoveJog(serial_number_.c_str(), channel_, mot_direction));
}

Error NRT100::StartDrive(Direction dir) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  if (dir != Direction::kForward && dir != Direction::kBackward)
    return Error::FromKinesis(FT_InvalidParameter);

  MOT_TravelDirection mot_direction;

  if (dir == Direction::kForward) {
    mot_direction = MOT_TravelDirection::MOT_Forwards;
  } else {
    mot_direction = MOT_TravelDirection::MOT_Reverse;
  }

  return Error::FromKinesis(
      SBC_MoveAtVelocity(serial_number_.c_str(), channel_, mot_direction));
}

Error NRT100::Stop(StopMode stop_mode) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  if (stop_mode != StopMode::kProfiled && stop_mode != StopMode::kImmediate)
    return Error::InvalidArgument("Invalid stop mode.");
  if (stop_mode == StopMode::kProfiled) {
    return Error::FromKinesis(
        SBC_StopProfiled(serial_number_.c_str(), channel_));
  }

  return Error::FromKinesis(
      SBC_StopImmediate(serial_number_.c_str(), channel_));
}

Error NRT100::StartMoveAbsolute(double position) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  if (!std::isfinite(position) || position < 0.0 || position > GetTravelMm())
    return Error::InvalidArgument(
        "Position is outside the selected stage travel.");
  int device_unit = 0;
  auto status = Error::FromKinesis(
      SBC_GetDeviceUnitFromRealValue(serial_number_.c_str(), channel_, position,
                                     &device_unit, kUnitTypeDistance));
  if (!status.ok()) return status;
  return Error::FromKinesis(
      SBC_MoveToPosition(serial_number_.c_str(), channel_, device_unit));
}
Error NRT100::StartMoveRelative(double distance) {
  auto connection = CheckConnection();
  if (!connection.ok()) return connection;
  if (!std::isfinite(distance) || distance < -GetTravelMm() ||
      distance > GetTravelMm()) {
    return Error::FromKinesis(FT_InvalidParameter);
  }

  auto position = GetPosition();
  if (!position) return position.error();
  if (*position + distance < 0.0 || *position + distance > GetTravelMm())
    return Error::InvalidArgument(
        "Relative target is outside the selected stage travel.");
  int device_units = 0;
  auto conversion_status = Error::FromKinesis(
      SBC_GetDeviceUnitFromRealValue(serial_number_.c_str(), channel_, distance,
                                     &device_units, kUnitTypeDistance));
  if (!conversion_status.ok()) {
    return conversion_status;
  }

  // A zero displacement may never produce the completion event the worker
  // awaits.
  if (device_units == 0) {
    return Error::MotionBelowResolution(serial_number_);
  }
  const short result =
      SBC_MoveRelative(serial_number_.c_str(), channel_, device_units);
  return Error::FromKinesis(result);
}

std::expected<MotorStatus, Error> NRT100::GetStatus() {
  auto status = CheckConnection();
  if (!status.ok()) {
    return std::unexpected(status);
  }

  const auto bits = SBC_GetStatusBits(serial_number_.c_str(), channel_);
  MotorStatus status_flags;
  status_flags.forward_limit_switch = (bits & 0x00000001u) != 0;
  status_flags.reverse_limit_switch = (bits & 0x00000002u) != 0;
  status_flags.moving_forward = (bits & 0x00000010u) != 0;
  status_flags.moving_reverse = (bits & 0x00000020u) != 0;
  status_flags.jogging_forward = (bits & 0x00000040u) != 0;
  status_flags.jogging_reverse = (bits & 0x00000080u) != 0;
  status_flags.homing = (bits & 0x00000200u) != 0;
  status_flags.homed = (bits & 0x00000400u) != 0;
  status_flags.active = (bits & 0x20000000u) != 0;
  status_flags.channel_enabled = (bits & 0x80000000u) != 0;
  return status_flags;
}

Error NRT100::CheckConnection() const {
  if (!connected_ || !SBC_CheckConnection(serial_number_.c_str())) {
    return Error::NotConnected(serial_number_);
  }
  return Error::Ok();
}

NRT100::EventResult NRT100::GetNextEvent() {
  auto status = CheckConnection();
  if (!status.ok()) {
    return std::unexpected(status);
  }

  WORD type = 0;
  WORD id = 0;
  DWORD data = 0;
  if (!SBC_GetNextMessage(serial_number_.c_str(), channel_, &type, &id,
                          &data)) {
    status = CheckConnection();
    if (!status.ok()) {
      return std::unexpected(status);
    }
    return std::optional<MotorEvent>{};
  }

  constexpr WORD kGenericMotorMessageType = 2;
  constexpr WORD kHomedMessageId = 0;
  constexpr WORD kMovedMessageId = 1;
  constexpr WORD kStoppedMessageId = 2;

  if (type == kGenericMotorMessageType) {
    switch (id) {
      case kHomedMessageId:
        return MotorEvent::kHomed;
      case kMovedMessageId:
        return MotorEvent::kMoveCompleted;
      case kStoppedMessageId:
        return MotorEvent::kStopped;
    }
  }
  return MotorEvent::kOther;
}

Error NRT100::ClearMessageQueue() {
  auto status = CheckConnection();
  if (!status.ok()) {
    return status;
  }
  SBC_ClearMessageQueue(serial_number_.c_str(), channel_);
  return Error::Ok();
}

}  // namespace thorlabs
