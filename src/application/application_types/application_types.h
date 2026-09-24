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

struct ScanConfiguration {
  algo::BinaryMatrix pattern;
  // Pixel mapped to the current physical X/Y position.
  algo::PixelPosition start_pixel;
  double pixel_size_mm = 0.0;  // Must be positive.
  std::chrono::milliseconds exposure_time{0};
  std::chrono::milliseconds motion_timeout{60000};
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
Q_DECLARE_METATYPE(application::AxisState)
Q_DECLARE_METATYPE(application::LaserState)
Q_DECLARE_METATYPE(application::ScanConfiguration)
Q_DECLARE_METATYPE(application::ScanState)

#endif  // APPLICATION_APPLICATION_TYPES_H_
