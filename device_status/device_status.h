#ifndef DEVICE_STATUS_H_
#define DEVICE_STATUS_H_

#include <string>
#include <optional>

namespace thorlabs {

enum class DeviceStatusCode {
  kOk,
  kKinesisError,
  kDeviceNotFound
};

class DeviceStatus {
    public:
        DeviceStatus() = delete;

        static DeviceStatus Ok();
        static DeviceStatus FromKinesis(short kinesis_error_code);
        static DeviceStatus DeviceNotFound(const std::string& serial_number);

        ~DeviceStatus() = default;

        [[nodiscard]] DeviceStatusCode error_code() const {return error_code_;}
        [[nodiscard]] std::optional<short> kinesis_error_code() const {return kinesis_error_code_;}
        [[nodiscard]] const std::string& error_message() const {return error_message_;}

        [[nodiscard]] bool ok() const {return error_code_ == DeviceStatusCode::kOk;}

    private:
        DeviceStatus(DeviceStatusCode error_code, std::optional<short> kinesis_error_code, std::string error_message);

        DeviceStatusCode error_code_;
        std::optional<short> kinesis_error_code_;
        std::string error_message_;
};

} // namesapce thorlabs

#endif // DEVICE_STATUS_H

