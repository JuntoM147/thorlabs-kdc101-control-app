#ifndef DEVICE_ERROR_H_
#define DEVICE_ERROR_H_

#include "device_error.h"

#include <string>

namespace thorlabs {

class DeviceError {
    public:
        DeviceError() = delete;

        explicit DeviceError(int error_code);

        std::string ErrMessage() const {return error_message_;}

        bool Ok() const {return error_code_ == 0;}

        ~DeviceError() = default;

    private:
        std::string error_message_;
        int error_code_;
};

} // namesapce thorlabs

#endif // DEVICE_ERROR_H_

