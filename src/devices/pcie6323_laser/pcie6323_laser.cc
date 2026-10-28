#include "pcie6323_laser.h"

#include <array>
#include <regex>
#include <utility>

namespace NI_DAQ {
namespace {
constexpr double kWriteTimeoutSeconds = 1.0;
Error ReadError(int32 code) {
  std::array<char, 4096> message{};
  DAQmxGetExtendedErrorInfo(message.data(),
                            static_cast<uInt32>(message.size()));
  return Error::FromNiDaq(code, message.data());
}
}  // namespace
Pcie6323Laser::CreateResult Pcie6323Laser::CreateLaser(
    const std::string& channel) {
  if (!PCIE6323_DIGITAL) {
    return std::unexpected(Error::InvalidArgument(
        "PCIe-6323 laser wiring is unconfigured. Confirm digital on/off wiring "
        "and configure PCIE6323_LASER_MODE=digital before connecting."));
  }
  std::smatch match;
  if (!std::regex_match(channel, match,
                        std::regex(R"(/?([^/,\s]+)/port[0-9]+/line[0-9]+)"))) {
    return std::unexpected(Error::InvalidArgument(
        "Enter one physical digital line matching the actual laser wiring."));
  }
  const auto device = match[1].str();
  std::array<char, 256> product{};
  auto status = DAQmxGetDevProductType(device.c_str(), product.data(),
                                       static_cast<uInt32>(product.size()));
  if (DAQmxFailed(status)) return std::unexpected(ReadError(status));
  if (std::string(product.data()) != "PCIe-6323") {
    return std::unexpected(
        Error::InvalidArgument("Selected device is not a PCIe-6323."));
  }
  uInt32 serial = 0;
  status = DAQmxGetDevSerialNum(device.c_str(), &serial);
  if (DAQmxFailed(status)) return std::unexpected(ReadError(status));
  if (serial != PCIE6323_SERIAL) {
    return std::unexpected(Error::InvalidArgument(
        "PCIe-6323 serial does not match PCIE6323_SERIAL_HEX."));
  }
  auto laser = std::unique_ptr<Pcie6323Laser>(new Pcie6323Laser());

  status = DAQmxCreateTask("", &laser->task_);
  if (DAQmxFailed(status)) {
    return std::unexpected(ReadError(status));
  }

  status = DAQmxCreateDOChan(laser->task_, channel.c_str(), "",
                             DAQmx_Val_ChanForAllLines);
  if (DAQmxFailed(status)) {
    return std::unexpected(ReadError(status));
  }

  uInt32 line_count = 0;
  status = DAQmxGetDONumLines(laser->task_, "", &line_count);
  if (DAQmxFailed(status)) {
    return std::unexpected(ReadError(status));
  }
  if (line_count != 1) {
    return std::unexpected(Error::InvalidLaserLineCount(channel, line_count));
  }

  laser->configured_ = true;
  auto result = laser->TurnOff();
  if (!result.ok()) {
    return std::unexpected(std::move(result));
  }

  return laser;
}
Pcie6323Laser::~Pcie6323Laser() noexcept {
  if (task_ != nullptr) {
    if (configured_) {
      DAQmxWriteDigitalScalarU32(task_, true, kWriteTimeoutSeconds, 0u,
                                 nullptr);
    }
    DAQmxClearTask(task_);
  }
}

Error Pcie6323Laser::Write(bool on) {
  const uInt32 value = on ? 1u : 0u;
  const int32 status = DAQmxWriteDigitalScalarU32(
      task_, true, kWriteTimeoutSeconds, value, nullptr);
  if (DAQmxFailed(status)) {
    return ReadError(status);
  }
  return Error::Ok();
}

Error Pcie6323Laser::TurnOn() { return Write(true); }

Error Pcie6323Laser::TurnOff() { return Write(false); }

}  // namespace NI_DAQ
