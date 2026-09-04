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
constexpr bool kDefaultSimulationStatus = true;

class KDC101 {
    public:
        KDC101() = delete;

        explicit KDC101(std::string serial_number, int polling_rate = kDefaultPollingRate, bool simulation_status = kDefaultSimulationStatus);

        DeviceError Home(double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        DeviceError Jog(Direction dir, double step_size = kDefaultStepSize, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        DeviceError StartDrive(Direction dir, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        DeviceError StopDrive();

        DeviceError MoveAbsolute(double position, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration);

        void Abort();

        ~KDC101() {}

    private:
        const std::string serial_number;
        bool is_simulation_;
};

} // namespace thorlabs 

