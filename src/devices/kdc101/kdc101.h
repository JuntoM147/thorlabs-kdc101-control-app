#ifndef KDC101_H_
#define KDC101_H_

#include <string>
#include <expected> // Requires C++23
#include <memory>
#include <cstdint>
#include <optional>

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

// Match the SDK's unsigned WORD (16-bit) and DWORD (32-bit) ranges
struct DeviceMessage {
    std::uint16_t type;
    std::uint16_t id;
    std::uint32_t data;
};

class KDC101 {
    public:
        using CreateResult = std::expected<std::unique_ptr<KDC101>, DeviceStatus>;
        using PositionResult = std::expected<double, DeviceStatus>;
        using MessageResult = std::expected<std::optional<DeviceMessage>, DeviceStatus>;

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

        [[nodiscard]] DeviceStatus StartHome(double speed = kDefaultSpeed);
        [[nodiscard]] DeviceStatus StartJog(Direction dir, double step_size = kDefaultStepSize, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);
        [[nodiscard]] DeviceStatus StartDrive(Direction dir, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);
        [[nodiscard]] DeviceStatus Stop(StopMode stop_mode = StopMode::kProfiled);
        [[nodiscard]] DeviceStatus StartMoveAbsolute(double position, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        [[nodiscard]] MessageResult GetNextMessage();
        [[nodiscard]] DeviceStatus CheckConnection() const;

        [[nodiscard]] DeviceStatus ClearMessageQueue();

    private:
        KDC101(std::string serial_number, bool simulation);

        const std::string serial_number_;
        bool simulation_;
        bool connected_;
        bool polling_;
        bool channel_enabled_;

};

} // namespace thorlabs 

#endif // KDC101_H_

