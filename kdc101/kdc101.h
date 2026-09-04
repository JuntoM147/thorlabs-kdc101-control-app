#ifndef KDC101_H_
#define KDC101_H_

#include "device_status/device_status.h"

namespace thorlabs {

enum class Direction {
    kForward = 0,
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
        KDC101() = delete;
        explicit KDC101(std::string serial_number, int polling_interval_ms = kDefaultPollingIntervalMs, bool simulation = false)
            : serial_number_{serial_number},
            simulation_{simulation},
            connected_{false} {}

        // kdc101 motor should not be copyable
        KDC101(const KDC101&) = delete;
        KDC101 operator=(const KDC101&) = delete;

        ~KDC101();
        
        [[nodiscard]] DeviceStatus Connect();

        [[nodiscard]] DeviceStatus GetPosition();

        [[nodiscard]] DeviceStatus Home(double speed = kDefaultSpeed);

        [[nodiscard]] DeviceStatus Jog(Direction dir, double step_size = kDefaultStepSize, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        [[nodiscard]] DeviceStatus StartDrive(Direction dir, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        [[nodiscard]] DeviceStatus Stop(StopMode stop_mode = StopMode::kProfiled);

        [[nodiscard]] DeviceStatus MoveAbsolute(double position, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

    private:
        const std::string serial_number_;
        bool simulation_;
        bool connected_;
};

} // namespace thorlabs 

#endif // KDC101_H_

