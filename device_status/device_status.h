#ifndef DEVICE_STATUS_H_
#define DEVICE_STATUS_H

#include <string>

namespace thorlabs {

class DeviceStatus {
    public:
        DeviceStatus() = delete;

        explicit DeviceStatus(int error_code);

        ~DeviceStatus() = default;

        [[nodiscard]] const std::string& error_message() const {return error_message_;}

        [[nodiscard]] bool ok() const {return error_code_ == 0;}

    private:
        std::string error_message_;
        int error_code_;
};

} // namesapce thorlabs

#endif // DEVICE_STATUS_H

