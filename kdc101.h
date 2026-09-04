#ifndef DEVICE_ERROR_H_
#define DEVICE_ERROR_H_

#include "device_error.h"

#endif // DEVICE_ERROR_H_


namespace thorlabs {

enum class Direction {
    kForwards = 0,
    kBackwards;
};

constexpr double kDefaultSpeed = 0.0;
constexpr double kDefaultAcceleration = 0.0;
constexpr double kDefaultStepSize = 0.0;
constexpr int kDefaultPollingRate = 200;

class KDC101 {
    public:
        KDC101() = delete;

        explicit KDC101(int serial_number, int polling_rate = kDefaultPollingRate) : serial_number_{serial_number} {}

        DeviceError Home(double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        DeviceError Jog(Direction dir, double step_size = kDefaultStepSize, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        DeviceError Drive(Direction dir, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        DeviceError MoveAbsolute(double position, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        void Abort();

        ~KDC101() {}

    private:
        const int serial_number_;
};

} // thorlabs 

