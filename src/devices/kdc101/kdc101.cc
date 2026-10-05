#include "kdc101/kdc101.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Thorlabs.MotionControl.KCube.DCServo.h"
#include "error/error.h"
#include "kinesis_simulation/kinesis_simulation.h"

namespace thorlabs {
using errors::Error;

constexpr int kDeviceID = 27;  // KDC101 Device ID (from Thorlabs Kinesis C API)
constexpr size_t kDeviceListBufferSize = 250;

namespace {

Error GetDevices(std::vector<std::string>& serial_numbers) {
  // Build list of connected devices
  Error build_list_status = Error::FromKinesis(TLI_BuildDeviceList());
  if (!build_list_status.ok()) {
    return build_list_status.WithContext("TLI_BuildDeviceList");
  }

  // get LTS serial numbers
  std::array<char, kDeviceListBufferSize> device_list_buffer{};
  Error device_list_status = Error::FromKinesis(TLI_GetDeviceListByTypeExt(
      device_list_buffer.data(), static_cast<DWORD>(device_list_buffer.size()),
      kDeviceID));
  if (!device_list_status.ok()) {
    return device_list_status.WithContext("TLI_GetDeviceListByTypeExt");
  }

  // populate vector with the serial numbers found
  std::string device_list(device_list_buffer.data());
  std::stringstream ss(device_list);
  std::string curr_serial_no;

  while (std::getline(ss, curr_serial_no, ',')) {
    serial_numbers.push_back(curr_serial_no);
  }

  return Error::Ok();
}

Error FindDevice(const std::string& serial_number) {
  std::vector<std::string> serial_numbers{};

  Error device_list_status = GetDevices(serial_numbers);
  if (!device_list_status.ok()) {
    return device_list_status;
  };

  const auto iter = std::ranges::find(serial_numbers, serial_number);

  if (iter == serial_numbers.end()) {
    return Error::DeviceNotFound(serial_number);
  }

  return Error::Ok();
}

Error OpenDevice(const std::string& serial_number) {
  Error open_status = Error::FromKinesis(CC_Open(serial_number.c_str()));
  if (!open_status.ok()) {
    return open_status.WithContext("CC_Open");
  }

  return Error::Ok();
}

Error LoadSettings(const std::string& serial_number) {
  // Use the known actuator profile, not a previously saved stage selection.
  if (!CC_LoadNamedSettings(serial_number.c_str(), kDefaultStageSettings)) {
    return Error::FailedToLoadSettings(serial_number)
        .WithContext("CC_LoadNamedSettings(Z825)");
  }

  return Error::Ok();
}

Error EnableChannel(const std::string& serial_number) {
  Error channel_status =
      Error::FromKinesis(CC_EnableChannel(serial_number.c_str()));
  if (!channel_status.ok()) {
    return channel_status.WithContext("CC_EnableChannel");
  }

  return Error::Ok();
}

Error StartPolling(const std::string& serial_number, int polling_interval_ms) {
  // Start the device polling
  if (!CC_StartPolling(serial_number.c_str(), polling_interval_ms)) {
    return Error::FailedToStartPolling(serial_number)
        .WithContext("CC_StartPolling");
  }

  return Error::Ok();
}

}  // namespace

KDC101::KDC101(std::string serial_number,
               std::shared_ptr<const KinesisSimulation> simulation)
    : serial_number_{serial_number},
      simulation_{std::move(simulation)},
      connected_{false},
      polling_{false},
      channel_enabled_{false} {}

KDC101::CreateResult KDC101::CreateMotor(
    std::string serial_number, int polling_interval_ms,
    std::shared_ptr<const KinesisSimulation> simulation) {
  auto device =
      std::unique_ptr<KDC101>(new KDC101(serial_number, std::move(simulation)));

  // Find device
  Error status = FindDevice(serial_number);
  if (!status.ok()) {
    return std::unexpected(status);
  }

  // Open device
  status = OpenDevice(serial_number);
  if (!status.ok()) {
    return std::unexpected(status);
  }

  device->connected_ = true;

  // Load Settings
  status = LoadSettings(serial_number);
  if (!status.ok()) {
    return std::unexpected(status);
  }

  // Enable Channel
  status = EnableChannel(serial_number);
  if (!status.ok()) {
    return std::unexpected(status);
  }

  device->channel_enabled_ = true;

  // Start Polling
  status = StartPolling(serial_number, polling_interval_ms);
  if (!status.ok()) {
    return std::unexpected(status);
  }

  device->polling_ = true;

  status = device->ApplyDefaults();
  if (!status.ok()) {
    return std::unexpected(status.WithContext("Apply KDC101 defaults"));
  }

  return device;
}

Error KDC101::ApplyDefaults() {
  auto status = SetMoveVelocity({kDefaultMoveSpeedMmPerSecond,
                                 kDefaultMoveAccelerationMmPerSecondSquared});
  if (!status.ok()) return status.WithContext("Set move velocity");

  status = SetJogVelocity(
      {kDefaultJogSpeedMmPerSecond, kDefaultJogAccelerationMmPerSecondSquared});
  if (!status.ok()) return status.WithContext("Set jog velocity");

  status = SetHomingSpeed(kDefaultHomingSpeedMmPerSecond);
  if (!status.ok()) return status.WithContext("Set homing speed");

  status = SetJogStepSize(kDefaultJogStepMm);
  if (!status.ok()) return status.WithContext("Set jog step");

  status = SetJogMode(JogMode::kSingleStep, StopMode::kProfiled);
  if (!status.ok()) return status.WithContext("Set jog mode");

  return SetBacklash(kDefaultBacklashMm).WithContext("Set backlash");
}

KDC101::~KDC101() {
  if (connected_) {
    CC_ClearMessageQueue(serial_number_.c_str());
    CC_StopProfiled(
        serial_number_.c_str());  // No point waiting for stop to finish
  }

  if (channel_enabled_) {
    CC_DisableChannel(serial_number_.c_str());
  }

  if (polling_) {
    CC_StopPolling(serial_number_.c_str());
  }

  if (connected_) {
    CC_Close(serial_number_.c_str());
  }
}

namespace {

constexpr int kUnitTypeDistance = 0;
constexpr int kUnitTypeSpeed = 1;
constexpr int kUnitTypeAcceleration = 2;

std::expected<int, Error> ConvertMotionParameter(
    const std::string& serial_number, double value, int unit_type) {
  if (!std::isfinite(value) || value <= 0.0) {
    return std::unexpected(Error::FromKinesis(FT_InvalidParameter));
  }
  int device_units = 0;
  auto status = Error::FromKinesis(CC_GetDeviceUnitFromRealValue(
      serial_number.c_str(), value, &device_units, unit_type));

  if (!status.ok()) {
    return std::unexpected(status);
  }

  if (device_units <= 0) {
    return std::unexpected(Error::FromKinesis(FT_InvalidParameter));
  }

  return device_units;
}

Error ApplyVelocity(const std::string& serial_number, VelocityParameters update,
                    bool jog) {
  std::optional<int> speed, acceleration;
  if (update.speed_mm_per_second) {
    auto converted = ConvertMotionParameter(
        serial_number, *update.speed_mm_per_second, kUnitTypeSpeed);
    if (!converted) return converted.error();
    speed = *converted;
  }
  if (update.acceleration_mm_per_second_squared) {
    auto converted = ConvertMotionParameter(
        serial_number, *update.acceleration_mm_per_second_squared,
        kUnitTypeAcceleration);
    if (!converted) return converted.error();
    acceleration = *converted;
  }
  if (!speed && !acceleration) return Error::Ok();
  int device_speed = 0, device_acceleration = 0;
  const char* serial = serial_number.c_str();
  auto status = Error::FromKinesis(
      jog ? CC_GetJogVelParams(serial, &device_acceleration, &device_speed)
          : CC_GetVelParams(serial, &device_acceleration, &device_speed));
  if (!status.ok()) return status;
  // Explicit updates must reach the controller even if the SDK's cached
  // parameters already match. Preserve only fields omitted by the caller.
  return Error::FromKinesis(
      jog ? CC_SetJogVelParams(serial,
                               acceleration.value_or(device_acceleration),
                               speed.value_or(device_speed))
          : CC_SetVelParams(serial, acceleration.value_or(device_acceleration),
                            speed.value_or(device_speed)));
}

}  // namespace

KDC101::PositionResult KDC101::GetPosition() {
  int device_unit = CC_GetPosition(serial_number_.c_str());

  double position_real;
  Error conversion_status = Error::FromKinesis(CC_GetRealValueFromDeviceUnit(
      serial_number_.c_str(), device_unit, &position_real, kUnitTypeDistance));

  if (!conversion_status.ok()) {
    return std::unexpected(conversion_status);
  }

  return position_real;
}

Error KDC101::SetMoveVelocity(VelocityParameters parameters) {
  return ApplyVelocity(serial_number_, parameters, false);
}

std::expected<MotorConfiguration, Error> KDC101::GetConfiguration() {
  auto status = CheckConnection();
  if (!status.ok()) return std::unexpected(status);
  int acceleration = 0, speed = 0;
  status = Error::FromKinesis(
      CC_GetVelParams(serial_number_.c_str(), &acceleration, &speed));
  if (!status.ok()) return std::unexpected(status);
  MotorConfiguration configuration{};
  const auto read = [this](int units, int type, double& value) {
    return Error::FromKinesis(CC_GetRealValueFromDeviceUnit(
        serial_number_.c_str(), units, &value, type));
  };
  status = read(acceleration, kUnitTypeAcceleration,
                configuration.acceleration_mm_per_second_squared);
  if (!status.ok()) return std::unexpected(status);
  status = read(CC_GetHomingVelocity(serial_number_.c_str()), kUnitTypeSpeed,
                configuration.homing_speed_mm_per_second);
  if (!status.ok()) return std::unexpected(status);
  status = read(CC_GetBacklash(serial_number_.c_str()), kUnitTypeDistance,
                configuration.backlash_mm);
  if (!status.ok()) return std::unexpected(status);
  return configuration;
}

KDC101::PositionResult KDC101::GetDistanceResolution() {
  double resolution = 0;
  const auto status = Error::FromKinesis(CC_GetRealValueFromDeviceUnit(
      serial_number_.c_str(), 1, &resolution, kUnitTypeDistance));
  if (!status.ok()) {
    return std::unexpected(status);
  }

  if (!std::isfinite(resolution) || resolution <= 0) {
    return std::unexpected(Error::FromKinesis(FT_InvalidParameter));
  }

  return resolution;
}

Error KDC101::SetJogVelocity(VelocityParameters parameters) {
  return ApplyVelocity(serial_number_, parameters, true);
}

Error KDC101::SetHomingSpeed(double speed) {
  auto converted =
      ConvertMotionParameter(serial_number_, speed, kUnitTypeSpeed);
  if (!converted) {
    return converted.error();
  }

  return Error::FromKinesis(CC_SetHomingVelocity(
      serial_number_.c_str(), static_cast<unsigned int>(*converted)));
}

Error KDC101::SetBacklash(double distance_mm) {
  if (!std::isfinite(distance_mm) || distance_mm < 0.0) {
    return Error::InvalidArgument(
        "Backlash must be a finite, non-negative distance in mm.");
  }
  if (distance_mm == 0.0) {
    return Error::FromKinesis(CC_SetBacklash(serial_number_.c_str(), 0));
  }

  auto converted =
      ConvertMotionParameter(serial_number_, distance_mm, kUnitTypeDistance);
  if (!converted) return converted.error();
  return Error::FromKinesis(CC_SetBacklash(serial_number_.c_str(), *converted));
}

Error KDC101::SetJogStepSize(double step) {
  auto converted =
      ConvertMotionParameter(serial_number_, step, kUnitTypeDistance);
  if (!converted) {
    return converted.error();
  }
  return Error::FromKinesis(CC_SetJogStepSize(
      serial_number_.c_str(), static_cast<unsigned int>(*converted)));
}

Error KDC101::SetJogMode(JogMode mode, StopMode stop_mode) {
  if ((mode != JogMode::kSingleStep && mode != JogMode::kContinuous) ||
      (stop_mode != StopMode::kProfiled && stop_mode != StopMode::kImmediate)) {
    return Error::FromKinesis(FT_InvalidParameter);
  }
  return Error::FromKinesis(CC_SetJogMode(
      serial_number_.c_str(),
      mode == JogMode::kSingleStep ? MOT_JogModes::MOT_SingleStep
                                   : MOT_JogModes::MOT_Continuous,
      stop_mode == StopMode::kProfiled ? MOT_StopModes::MOT_Profiled
                                       : MOT_StopModes::MOT_Immediate));
}

Error KDC101::StartHome() {
  return Error::FromKinesis(CC_Home(serial_number_.c_str()));
}

Error KDC101::StartJog(Direction dir) {
  if (dir != Direction::kForward && dir != Direction::kBackward)
    return Error::FromKinesis(FT_InvalidParameter);
  MOT_TravelDirection mot_direction;

  if (dir == Direction::kForward) {
    mot_direction = MOT_TravelDirection::MOT_Forwards;
  } else {
    mot_direction = MOT_TravelDirection::MOT_Backwards;
  }

  return Error::FromKinesis(CC_MoveJog(serial_number_.c_str(), mot_direction));
}

Error KDC101::StartDrive(Direction dir) {
  if (dir != Direction::kForward && dir != Direction::kBackward)
    return Error::FromKinesis(FT_InvalidParameter);

  MOT_TravelDirection mot_direction;

  if (dir == Direction::kForward) {
    mot_direction = MOT_TravelDirection::MOT_Forwards;
  } else {
    mot_direction = MOT_TravelDirection::MOT_Backwards;
  }

  return Error::FromKinesis(
      CC_MoveAtVelocity(serial_number_.c_str(), mot_direction));
}

Error KDC101::Stop(StopMode stop_mode) {
  if (stop_mode == StopMode::kProfiled) {
    return Error::FromKinesis(CC_StopProfiled(serial_number_.c_str()));
  }

  return Error::FromKinesis(CC_StopImmediate(serial_number_.c_str()));
}

Error KDC101::StartMoveAbsolute(double position) {
  if (!std::isfinite(position)) return Error::FromKinesis(FT_InvalidParameter);
  int device_unit = 0;
  auto status = Error::FromKinesis(CC_GetDeviceUnitFromRealValue(
      serial_number_.c_str(), position, &device_unit, kUnitTypeDistance));
  if (!status.ok()) return status;
  return Error::FromKinesis(
      CC_MoveToPosition(serial_number_.c_str(), device_unit));
}
Error KDC101::StartMoveRelative(double distance) {
  if (!std::isfinite(distance)) {
    return Error::FromKinesis(FT_InvalidParameter);
  }

  int device_units = 0;
  auto conversion_status = Error::FromKinesis(CC_GetDeviceUnitFromRealValue(
      serial_number_.c_str(), distance, &device_units, kUnitTypeDistance));
  if (!conversion_status.ok()) {
    return conversion_status;
  }

  // A zero displacement may never produce the completion event the worker
  // awaits.
  if (device_units == 0) {
    return Error::MotionBelowResolution(serial_number_);
  }
  const short result = CC_MoveRelative(serial_number_.c_str(), device_units);
  return Error::FromKinesis(result);
}

std::expected<MotorStatus, Error> KDC101::GetStatus() {
  auto status = CheckConnection();
  if (!status.ok()) {
    return std::unexpected(status);
  }

  const auto bits = CC_GetStatusBits(serial_number_.c_str());
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

Error KDC101::CheckConnection() const {
  if (!connected_ || !CC_CheckConnection(serial_number_.c_str())) {
    return Error::NotConnected(serial_number_);
  }
  return Error::Ok();
}

KDC101::EventResult KDC101::GetNextEvent() {
  auto status = CheckConnection();
  if (!status.ok()) {
    return std::unexpected(status);
  }

  WORD type = 0;
  WORD id = 0;
  DWORD data = 0;
  if (!CC_GetNextMessage(serial_number_.c_str(), &type, &id, &data)) {
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

Error KDC101::ClearMessageQueue() {
  auto status = CheckConnection();
  if (!status.ok()) {
    return status;
  }
  CC_ClearMessageQueue(serial_number_.c_str());
  return Error::Ok();
}

}  // namespace thorlabs
