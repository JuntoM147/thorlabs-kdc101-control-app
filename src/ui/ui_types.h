#ifndef UI_UI_TYPES_H_
#define UI_UI_TYPES_H_

#include <string>

#include "algo/algorithm/algorithm.h"
#include "workers/worker_types.h"

namespace ui {
using Axis = workers::Axis;
using MotorSettings = workers::MotorSettings;
using Direction = thorlabs::Direction;
using StopMode = thorlabs::StopMode;
using ScanPhase = workers::ScanPhase;
using ScanState = workers::ScanProgress;

enum class ConnectionState { kDisconnected, kConnecting, kConnected, kFaulted };
enum class OperationState {
  kIdle,
  kHoming,
  kMoving,
  kJogging,
  kDriving,
  kStopping
};

// Presentation state: the window adds the axis identity to worker observations.
struct AxisState {
  Axis axis = Axis::kX;
  ConnectionState connection = ConnectionState::kDisconnected;
  OperationState operation = OperationState::kIdle;
  std::optional<double> position_mm;
  std::optional<bool> homed;
};
struct LaserState {
  ConnectionState connection = ConnectionState::kDisconnected;
  std::optional<bool> output_enabled;
};
struct MotorConnection {
  Axis axis;
  std::string serial_number;
  bool simulation = false;
};
struct LaserConnection {
  std::string digital_output_channel;
};
struct OperationError {
  std::optional<Axis> axis;
  std::string operation;
  std::string message;
};
struct ScanConfiguration {
  algo::BinaryMatrix pattern;
  algo::PixelPosition start_pixel;
  algo::Direction direction = algo::Direction::kPositiveX;
  double pixel_size_mm = 0.00025;
  std::chrono::milliseconds exposure_time{10};
};
}  // namespace ui

#endif  // UI_UI_TYPES_H_
