#ifndef WORKERS_WORKER_TYPES_H_
#define WORKERS_WORKER_TYPES_H_

#include <QMetaType>
#include <QtGlobal>
#include <chrono>
#include <cstddef>
#include <optional>

#include "algo/instructions/instructions.h"
#include "devices/kdc101/kdc101.h"
#include "error/error.h"

namespace workers {

// The caller assigns a nonzero ID to each request to match its result.
using RequestId = quint64;
enum class Axis { kX, kY, kZ };

// Missing fields leave the current device setting unchanged.
struct MotorSettings {
  thorlabs::VelocityParameters move;
  thorlabs::VelocityParameters jog;
  std::optional<double> homing_speed_mm_per_second;
  std::optional<double> jog_step_mm;
  std::optional<double> backlash_mm;
};

struct MotorState {
  bool connected = false;
  std::optional<double> position_mm;
  std::optional<thorlabs::MotorStatus> status;
};

struct LaserState {
  bool connected = false;
  // Unknown until the device confirms a write; disconnected is also unknown.
  std::optional<bool> output_enabled;
};

struct ScanJob {
  algo::Program instructions;
  // Instructions already contain physical targets scaled by pixel size.
  std::chrono::milliseconds pixel_exposure{10};  // Isolated pixel dwell.
  std::chrono::milliseconds operation_timeout{30000};
  // Slow drawing can legitimately take minutes; laser writes/cleanup retain
  // the shorter operation timeout above.
  std::chrono::milliseconds motion_timeout{std::chrono::hours(1)};
};

enum class ScanPhase { kIdle, kRunning, kPausing, kPaused, kStopping, kFailed };

struct ScanProgress {
  ScanPhase phase = ScanPhase::kIdle;
  std::size_t completed_instructions = 0;
  std::size_t total_instructions = 0;
  std::optional<std::chrono::seconds> estimated_remaining;
};

// Call once before creating queued connections between workers.
void RegisterWorkerMetaTypes();

}  // namespace workers

Q_DECLARE_METATYPE(errors::Error)
Q_DECLARE_METATYPE(thorlabs::Direction)
Q_DECLARE_METATYPE(thorlabs::StopMode)
Q_DECLARE_METATYPE(workers::Axis)
Q_DECLARE_METATYPE(workers::MotorSettings)
Q_DECLARE_METATYPE(workers::MotorState)
Q_DECLARE_METATYPE(workers::LaserState)
Q_DECLARE_METATYPE(workers::ScanJob)
Q_DECLARE_METATYPE(workers::ScanProgress)

#endif  // WORKERS_WORKER_TYPES_H_
