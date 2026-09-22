#include "ni_daq_laser.h"

#include <array>
#include <utility>

namespace NI_DAQ {
namespace {

constexpr double kWriteTimeoutSeconds = 1.0;

DeviceStatus ReadError(int32 code)
{
    std::array<char, 4096> message{};
    DAQmxGetExtendedErrorInfo(message.data(), static_cast<uInt32>(message.size()));
    return DeviceStatus::FromNiDaq(code, message.data());
}

} // namespace

Laser::CreateResult Laser::CreateLaser(const std::string& digital_output_channel)
{
    auto laser = std::unique_ptr<Laser>(new Laser());

    int32 status = DAQmxCreateTask("", &laser->task_);
    if (DAQmxFailed(status)) {
        return std::unexpected(ReadError(status));
    }

    status = DAQmxCreateDOChan(laser->task_, digital_output_channel.c_str(), "", DAQmx_Val_ChanForAllLines);
    if (DAQmxFailed(status)) {
        return std::unexpected(ReadError(status));
    }

    uInt32 line_count = 0;
    status = DAQmxGetDONumLines(laser->task_, "", &line_count);
    if (DAQmxFailed(status)) {
        return std::unexpected(ReadError(status));
    }
    if (line_count != 1) {
        return std::unexpected(DeviceStatus::InvalidLaserLineCount(digital_output_channel, line_count));
    }

    laser->configured_ = true;
    auto result = laser->TurnOff();
    if (!result.ok()) {
        return std::unexpected(std::move(result));
    }

    return laser;
}

Laser::~Laser() noexcept
{
    if (task_ != nullptr) {
        if (configured_) {
            DAQmxWriteDigitalScalarU32(task_, true, kWriteTimeoutSeconds, 0u, nullptr);
        }
        DAQmxClearTask(task_);
    }
}

DeviceStatus Laser::Write(bool on)
{
    const uInt32 value = on ? 1u : 0u;
    const int32 status = DAQmxWriteDigitalScalarU32(task_, true, kWriteTimeoutSeconds, value, nullptr);
    if (DAQmxFailed(status)) {
        return ReadError(status);
    }
    return DeviceStatus::Ok();
}

DeviceStatus Laser::TurnOn()
{
    return Write(true);
}

DeviceStatus Laser::TurnOff()
{
    return Write(false);
}

} // namespace NI_DAQ
