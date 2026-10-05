#ifndef ERROR_ERROR_H_
#define ERROR_ERROR_H_

#include <cstdint>
#include <optional>
#include <string>

namespace errors {

enum class ErrorCode {
  kOk,
  kKinesisError,
  kDeviceNotFound,
  kPollingError,
  kLoadSettingsError,
  kConnectionError,
  kTimeout,
  kNiDaqError,
  kLaserLineConfigurationError,
  kInvalidArgument,
  kBusy,
  kControlDenied,
  kCancelled,
  kMotionResolutionError
};

class Error {
 public:
  Error() = delete;

  static Error Ok();
  static Error InvalidArgument(const std::string& message);
  static Error Failure(ErrorCode code, const std::string& message);
  static Error FromKinesis(short kinesis_error_code);
  static Error FromNiDaq(std::int32_t code, const std::string& message);
  static Error InvalidLaserLineCount(const std::string& line,
                                     std::uint32_t count);
  static Error DeviceNotFound(const std::string& serial_number);
  static Error FailedToLoadSettings(const std::string& serial_number);
  static Error FailedToStartPolling(const std::string& serial_number);
  static Error NotConnected(const std::string& serial_number);
  static Error Timeout(const std::string& serial_number, int expected_message);
  static Error MotionBelowResolution(const std::string& serial_number);

  ~Error() = default;

  [[nodiscard]] ErrorCode error_code() const { return error_code_; }
  [[nodiscard]] std::optional<short> kinesis_error_code() const {
    return kinesis_error_code_;
  }
  [[nodiscard]] std::optional<std::int32_t> ni_daq_error_code() const {
    return ni_daq_error_code_;
  }
  [[nodiscard]] const std::string& error_message() const {
    return error_message_;
  }

  [[nodiscard]] bool ok() const { return error_code_ == ErrorCode::kOk; }
  [[nodiscard]] Error WithContext(const std::string& context) const;
  [[nodiscard]] Error WithDevice(const std::string& device) const;
  [[nodiscard]] const std::string& device() const { return device_; }

 private:
  Error(ErrorCode error_code, std::optional<short> kinesis_error_code,
        std::string error_message);

  ErrorCode error_code_;
  std::optional<short> kinesis_error_code_;
  std::optional<std::int32_t> ni_daq_error_code_;
  std::string error_message_;
  std::string device_;
};

}  // namespace errors

#endif  // ERROR_ERROR_H_
