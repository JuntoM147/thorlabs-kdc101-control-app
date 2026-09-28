#ifndef APPLICATION_APPLICATION_TYPES_H_
#define APPLICATION_APPLICATION_TYPES_H_

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

#include <QMetaType>

#include "../../algo/convert/conversion.h"
#include "../../algo/instructions/instructions.h"

namespace application {

// UI-facing inputs
enum class Axis { kX, kY, kZ };
enum class Direction { kForward, kBackward };
enum class StopMode { kProfiled, kImmediate };
enum class ConnectionState { kDisconnected, kConnecting, kConnected, kFaulted };
enum class OperationState { kIdle, kHoming, kMoving, kJogging, kDriving, kStopping };
enum class ScanPhase { kIdle, kWaiting, kPreparing, kRunning, kPauseRequested, kPausing, kPaused, kResuming, kStopping, kFailed };

struct MotorConnection {
  Axis axis = Axis::kX;
  std::string serial_number;
  int polling_interval_ms = 200;
};

struct LaserConnection {
  std::string digital_output_channel = "Dev1/port0/line0";
};

struct MotionSettings {
  // Empty means use the configured device setting.
  std::optional<double> speed_mm_per_second;
  std::optional<double> acceleration_mm_per_second_squared;
};

enum class JogMode { kSingleStep, kContinuous };

struct MotorSettings {
  MotionSettings move;
  MotionSettings jog;
  std::optional<double> homing_speed_mm_per_second;
  std::optional<double> jog_step_mm;
  std::optional<JogMode> jog_mode;
  StopMode jog_stop_mode = StopMode::kProfiled;
};

// Applied on every connection, after loading the stage's Kinesis profile.
inline constexpr double kDefaultJogStepMm = 0.005;
inline constexpr double kDefaultSpeedMmPerSecond = 0.08;
inline constexpr double kDefaultAccelerationMmPerSecondSquared = 1.5;
inline constexpr std::chrono::milliseconds kDefaultMotionTimeout = std::chrono::minutes(15);
inline MotorSettings DefaultMotorSettings() {
  MotorSettings settings;
  settings.move = {kDefaultSpeedMmPerSecond, kDefaultAccelerationMmPerSecondSquared};
  settings.jog = settings.move;
  settings.homing_speed_mm_per_second = kDefaultSpeedMmPerSecond;
  settings.jog_step_mm = kDefaultJogStepMm;
  settings.jog_mode = JogMode::kSingleStep;
  settings.jog_stop_mode = StopMode::kProfiled;
  return settings;
}

struct ScanConfiguration {
  bool motion_diagnostic = false; // Fixed laser-OFF sequence; retains endpoint checks.
  algo::BinaryMatrix pattern;
  // Pixel mapped to the current physical X/Y position.
  algo::PixelPosition start_pixel;
  double pixel_size_mm = 0.0;  // Must be positive.
  std::chrono::milliseconds exposure_time{0};
  std::chrono::milliseconds motion_timeout{kDefaultMotionTimeout};
  MotionSettings motion;
};

// Notifications sent back to the UI
struct OperationError {
  std::optional<Axis> axis;
  std::string operation;
  std::string message;
};

struct AxisState {
  Axis axis = Axis::kX;
  ConnectionState connection = ConnectionState::kDisconnected;
  OperationState operation = OperationState::kIdle;
  // Empty observations mean unavailable or stale.
  std::optional<double> position_mm;
  std::optional<bool> homed;
  std::optional<bool> forward_limit;
  std::optional<bool> reverse_limit;
  // Hardware feedback, independent of the application's pending command.
  std::optional<bool> hardware_moving;
  std::optional<bool> channel_enabled;
};

struct LaserState {
  ConnectionState connection = ConnectionState::kDisconnected;
  // Last successful output write, not physical emission feedback.
  std::optional<bool> output_enabled;
};

struct ScanState {
  ScanPhase phase = ScanPhase::kIdle;
  std::size_t completed_instructions = 0;
  std::size_t total_instructions = 0;
};

}  // namespace application

// Tell Qt about our types
Q_DECLARE_METATYPE(application::Axis)
Q_DECLARE_METATYPE(application::Direction)
Q_DECLARE_METATYPE(application::StopMode)
Q_DECLARE_METATYPE(application::OperationError)
Q_DECLARE_METATYPE(application::MotorConnection)
Q_DECLARE_METATYPE(application::LaserConnection)
Q_DECLARE_METATYPE(application::MotionSettings)
Q_DECLARE_METATYPE(application::MotorSettings)
Q_DECLARE_METATYPE(application::AxisState)
Q_DECLARE_METATYPE(application::LaserState)
Q_DECLARE_METATYPE(application::ScanConfiguration)
Q_DECLARE_METATYPE(application::ScanState)

#endif  // APPLICATION_APPLICATION_TYPES_H_
