#ifndef KDC101_H_
#define KDC101_H_

#include <expected>  // Requires C++23
#include <memory>
#include <optional>
#include <string>

#include "error/error.h"
#include "kdc101_defaults.h"
#include "kinesis_simulation/kinesis_simulation.h"

namespace thorlabs {
using errors::Error;

enum class Direction { kForward, kBackward };

enum class StopMode { kProfiled, kImmediate };

enum class JogMode { kSingleStep, kContinuous };

// Missing fields preserve the current device values
struct VelocityParameters {
  std::optional<double> speed_mm_per_second;
  std::optional<double> acceleration_mm_per_second_squared;
};

struct MotorConfiguration {
  double acceleration_mm_per_second_squared;
  double homing_speed_mm_per_second;
  double backlash_mm;
};

// Represents device messages from the message queue
enum class MotorEvent { kHomed, kMoveCompleted, kStopped, kOther };

// Represents device information from polling
struct MotorStatus {
  bool forward_limit_switch = false;  // Forward hardware travel limit triggered
  bool reverse_limit_switch = false;  // Reverse hardware travel limit triggered
  bool moving_forward = false;
  bool moving_reverse = false;
  bool jogging_forward = false;
  bool jogging_reverse = false;
  bool homing = false;
  bool homed = false;
  bool active = false;
  bool channel_enabled = false;
};

class KDC101 {
 public:
  using CreateResult = std::expected<std::unique_ptr<KDC101>, Error>;
  using PositionResult = std::expected<double, Error>;
  using EventResult = std::expected<std::optional<MotorEvent>, Error>;

  KDC101() = delete;
  ~KDC101();

  // kdc101 motor should not be copyable
  KDC101(const KDC101&) = delete;
  KDC101& operator=(const KDC101&) = delete;

  // kdc101 motor not movable
  KDC101(const KDC101&&) = delete;
  KDC101& operator=(const KDC101&&) = delete;

  [[nodiscard]] static CreateResult CreateMotor(
      std::string serial_number,
      int polling_interval_ms = kDefaultPollingIntervalMs,
      std::shared_ptr<const KinesisSimulation> simulation = nullptr);

  [[nodiscard]] PositionResult GetPosition();
  [[nodiscard]] PositionResult GetDistanceResolution();
  [[nodiscard]] std::expected<MotorStatus, Error> GetStatus();
  [[nodiscard]] std::expected<MotorConfiguration, Error> GetConfiguration();

  // Configuration only, doesn't start movement
  [[nodiscard]] Error SetMoveVelocity(VelocityParameters parameters);
  [[nodiscard]] Error SetJogVelocity(VelocityParameters parameters);
  [[nodiscard]] Error SetHomingSpeed(double speed_mm_per_second);
  [[nodiscard]] Error SetJogStepSize(double step_mm);
  [[nodiscard]] Error SetJogMode(JogMode mode, StopMode stop_mode);
  [[nodiscard]] Error SetBacklash(double distance_mm);

  // Methods to start a motor operation (non-blocking)
  [[nodiscard]] Error StartHome();
  [[nodiscard]] Error StartJog(Direction dir);
  [[nodiscard]] Error StartDrive(Direction dir);
  [[nodiscard]] Error Stop(StopMode stop_mode = StopMode::kProfiled);
  [[nodiscard]] Error StartMoveAbsolute(double position);
  [[nodiscard]] Error StartMoveRelative(double distance);

  [[nodiscard]] EventResult GetNextEvent();
  [[nodiscard]] Error CheckConnection() const;

  [[nodiscard]] Error ClearMessageQueue();

 private:
  [[nodiscard]] Error ApplyDefaults();
  KDC101(std::string serial_number,
         std::shared_ptr<const KinesisSimulation> simulation);

  const std::string serial_number_;
  std::shared_ptr<const KinesisSimulation> simulation_;
  bool connected_;
  bool polling_;
  bool channel_enabled_;
};

}  // namespace thorlabs

#endif  // KDC101_H_
