#ifndef KDC101_H_
#define KDC101_H_

#include <string>
#include <expected> // Requires C++23
#include <memory>
#include <optional>

#include "device_status/device_status.h"
#include "kinesis_simulation/kinesis_simulation.h"

namespace thorlabs {

enum class Direction {
    kForward,
    kBackward
};

enum class StopMode {
    kProfiled,
    kImmediate
};

constexpr double kDefaultSpeed = 0.0;
constexpr double kDefaultAcceleration = 0.0;
constexpr double kDefaultStepSize = 0.0;
constexpr int kDefaultPollingIntervalMs = 200;

// Represents device messages from the message queue
enum class MotorEvent {
    kHomed,
    kMoveCompleted,
    kStopped,
    kOther
};

// Represents device information from polling
struct MotorStatus {
    bool forward_limit_switch = false; // Forward hardware travel limit triggered
    bool reverse_limit_switch = false; // Reverse hardware travel limit triggered
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
        using CreateResult = std::expected<std::unique_ptr<KDC101>, DeviceStatus>;
        using PositionResult = std::expected<double, DeviceStatus>;
        using EventResult = std::expected<std::optional<MotorEvent>, DeviceStatus>;

        KDC101() = delete;
        ~KDC101();

        // kdc101 motor should not be copyable
        KDC101(const KDC101&) = delete;
        KDC101& operator=(const KDC101&) = delete;

        // kdc101 motor not movable
        KDC101(const KDC101&&) = delete;
        KDC101& operator=(const KDC101&&) = delete;
        
        [[nodiscard]] static CreateResult Create(std::string serial_number, int polling_interval_ms = kDefaultPollingIntervalMs, std::shared_ptr<const KinesisSimulation> simulation = nullptr);

        [[nodiscard]] PositionResult GetPosition();
        [[nodiscard]] std::expected<MotorStatus, DeviceStatus> GetStatus();

        // Methods to start a motor operation (non-blocking)
        [[nodiscard]] DeviceStatus StartHome(double speed = kDefaultSpeed);
        [[nodiscard]] DeviceStatus StartJog(Direction dir, double step_size = kDefaultStepSize, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);
        [[nodiscard]] DeviceStatus StartDrive(Direction dir, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);
        [[nodiscard]] DeviceStatus Stop(StopMode stop_mode = StopMode::kProfiled);
        [[nodiscard]] DeviceStatus StartMoveAbsolute(double position, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        [[nodiscard]] EventResult GetNextEvent();
        [[nodiscard]] DeviceStatus CheckConnection() const;

        [[nodiscard]] DeviceStatus ClearMessageQueue();

    private:
        KDC101(std::string serial_number, std::shared_ptr<const KinesisSimulation> simulation);

        const std::string serial_number_;
        std::shared_ptr<const KinesisSimulation> simulation_;
        bool connected_;
        bool polling_;
        bool channel_enabled_;
};

} // namespace thorlabs 

#endif // KDC101_H_

