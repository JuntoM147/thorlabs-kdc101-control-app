#ifndef DEVICE_STATUS_H_
#define DEVICE_STATUS_H_

#include <string>
#include <optional>
#include <cstdint>

namespace thorlabs {

enum class DeviceStatusCode {
  kOk,
  kKinesisError,
  kDeviceNotFound,
  kPollingError,  
  kLoadSettingsError,
  kConnectionError,
  kTimeout,
  kNiDaqError,
  kLaserLineConfigurationError
};

class DeviceStatus {
    public:
        DeviceStatus() = delete;

        static DeviceStatus Ok();
        static DeviceStatus FromKinesis(short kinesis_error_code);
        static DeviceStatus FromNiDaq(std::int32_t code, const std::string& message);
        static DeviceStatus InvalidLaserLineCount(const std::string& line, std::uint32_t count);
        static DeviceStatus DeviceNotFound(const std::string& serial_number);
        static DeviceStatus FailedToLoadSettings(const std::string& serial_number);
        static DeviceStatus FailedToStartPolling(const std::string& serial_number);
        static DeviceStatus NotConnected(const std::string& serial_number);
        static DeviceStatus Timeout(const std::string& serial_number, int expected_message);

        ~DeviceStatus() = default;

        [[nodiscard]] DeviceStatusCode error_code() const {return error_code_;}
        [[nodiscard]] std::optional<short> kinesis_error_code() const {return kinesis_error_code_;}
        [[nodiscard]] std::optional<std::int32_t> ni_daq_error_code() const {return ni_daq_error_code_;}
        [[nodiscard]] const std::string& error_message() const {return error_message_;}

        [[nodiscard]] bool ok() const {return error_code_ == DeviceStatusCode::kOk;}

    private:
        DeviceStatus(DeviceStatusCode error_code, std::optional<short> kinesis_error_code, std::string error_message);

        DeviceStatusCode error_code_;
        std::optional<short> kinesis_error_code_;
        std::optional<std::int32_t> ni_daq_error_code_;
        std::string error_message_;
};

} // namesapce thorlabs

#endif // DEVICE_STATUS_H

