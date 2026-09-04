#ifndef KDC101_H_
#define KDC101_H_

#include "kdc101/kdc101.h"

#include "device_error/device_error.h"
#include "Thorlabs.MotionControl.KCube.DCServo.h"

namespace thorlabs {

enum class Direction {
    kForwards = 0,
    kBackwards
};

enum class StopMode {
    kStopProfiled,
    kStopImmediate
};

constexpr double kDefaultSpeed = 0.0;
constexpr double kDefaultAcceleration = 0.0;
constexpr double kDefaultStepSize = 0.0;
constexpr int kDefaultPollingRate = 200;

class KDC101 {
    public:
        KDC101() = delete;

        ~KDC101();

        explicit KDC101(std::string serial_number, int polling_interval_ms = kDefaultPollingRate, bool simulation = false);

        [[nodiscard]] DeviceError Home(double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        [[nodiscard]] DeviceError Jog(Direction dir, double step_size = kDefaultStepSize, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        [[nodiscard]] DeviceError StartDrive(Direction dir, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        [[nodiscard]] DeviceError Stop(StopMode stop_mode = StopMode::kStopProfiled);

        [[nodiscard]] DeviceError MoveAbsolute(double position, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

    private:
        const std::string serial_number;
        bool simulation_;
};

} // namespace thorlabs 

#endif // KDC101_H_

