#include <string>

namespace thorlabs {

class DeviceError {
    public:
        DeviceError() = delete;

        explicit DeviceError(int error_code);

        std::string ErrMessage() const {return error_message_;}

        ~DeviceError() = default;

    private:
        std::string error_message_;
};

} // namesapce thorlabs

