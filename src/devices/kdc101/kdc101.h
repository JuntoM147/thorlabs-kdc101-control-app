#ifndef KDC101_H_
#define KDC101_H_

#include <string>
#include <expected> // Requires C++23
#include <memory>
#include <chrono>

#include "device_status/device_status.h"

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

class KDC101 {
    public:
        using CreateResult = std::expected<std::unique_ptr<KDC101>, DeviceStatus>;
        using PositionResult = std::expected<double, DeviceStatus>;

        KDC101() = delete;
        ~KDC101();

        // kdc101 motor should not be copyable
        KDC101(const KDC101&) = delete;
        KDC101& operator=(const KDC101&) = delete;

        // kdc101 motor not movable
        KDC101(const KDC101&&) = delete;
        KDC101& operator=(const KDC101&&) = delete;
        
        [[nodiscard]] static CreateResult Create(std::string serial_number, int polling_interval_ms = kDefaultPollingIntervalMs, bool simulation = false);

        [[nodiscard]] PositionResult GetPosition();
        [[nodiscard]] DeviceStatus Home(double speed = kDefaultSpeed);
        [[nodiscard]] DeviceStatus Jog(Direction dir, double step_size = kDefaultStepSize, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);
        [[nodiscard]] DeviceStatus StartDrive(Direction dir, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);
        [[nodiscard]] DeviceStatus Stop(StopMode stop_mode = StopMode::kProfiled);
        [[nodiscard]] DeviceStatus MoveAbsolute(double position, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

    private:
        KDC101(std::string serial_number, bool simulation);

        const std::string serial_number_;
        bool simulation_;
        bool connected_;
        bool polling_;
        bool channel_enabled_;

        [[nodiscard]] DeviceStatus WaitForMotorMessage(int expected_message, std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{2000});
};

} // namespace thorlabs 

#endif // KDC101_H_

