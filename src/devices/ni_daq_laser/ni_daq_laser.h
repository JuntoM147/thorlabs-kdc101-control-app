#ifndef NI_DAQ_LASER_H_
#define NI_DAQ_LASER_H_

#include <expected>
#include <memory>
#include <string>

#include <NIDAQmx.h>
#include "device_status/device_status.h"

namespace NI_DAQ {

using thorlabs::DeviceStatus;

class Laser {
public:
    using CreateResult = std::expected<std::unique_ptr<Laser>, DeviceStatus>;

    [[nodiscard]] static CreateResult CreateLaser(const std::string& digital_output_channel = "Dev1/port0/line0");

    ~Laser() noexcept;
    
    // Laser should not be copyable or movable
    Laser(const Laser&) = delete;
    Laser& operator=(const Laser&) = delete;
    Laser(Laser&&) = delete;
    Laser& operator=(Laser&&) = delete;

    [[nodiscard]] DeviceStatus TurnOn();
    [[nodiscard]] DeviceStatus TurnOff();

private:
    Laser() = default;
    [[nodiscard]] DeviceStatus Write(bool on);

    TaskHandle task_ = nullptr; // pointer to NI-DAQmx task for controlling the laser
    bool configured_ = false; // Records whether the laser is configured (for proper destruction)
};

} // namespace NI_DAQ

#endif // NI_DAQ_LASER_H_
