#include "device_status.h"

#include <string>
#include <optional>
 

namespace thorlabs {

namespace {

std::string KinesisErrorMessage(short error_code) {
  switch (error_code) {
    // FTDI and communication errors.
    case 1:
      return "FT_InvalidHandle: The FTDI functions have not been initialized.";
    case 2:
      return "FT_DeviceNotFound: The device could not be found. Ensure TLI_BuildDeviceList() has been called.";
    case 3:
      return "FT_DeviceNotOpened: The device must be opened before it can be accessed.";
    case 4:
      return "FT_IOError: An I/O error occurred in the FTDI communication interface.";
    case 5:
      return "FT_InsufficientResources: Insufficient resources are available.";
    case 6:
      return "FT_InvalidParameter: An invalid parameter was supplied.";
    case 7:
      return "FT_DeviceNotPresent: The device is no longer connected.";
    case 8:
      return "FT_IncorrectDevice: The detected device does not match the expected device.";

    // Device-library errors.
    case 16:
      return "FT_NoDLLLoaded: The required device library could not be found.";
    case 17:
      return "FT_NoFunctionsAvailable: No functions are available for this device.";
    case 18:
      return "FT_FunctionNotAvailable: The requested function is not available for this device.";
    case 19:
      return "FT_BadFunctionPointer: A bad function pointer was detected.";
    case 20:
      return "FT_GenericFunctionFail: The function failed to complete successfully.";
    case 21:
      return "FT_SpecificFunctionFail: The function failed to complete successfully.";

    // General DLL control errors.
    case 32:
      return "TL_ALREADY_OPEN: The device is already open.";
    case 33:
      return "TL_NO_RESPONSE: The device has stopped responding.";
    case 34:
      return "TL_NOT_IMPLEMENTED: The requested function has not been implemented.";
    case 35:
      return "TL_FAULT_REPORTED: The device reported a fault.";
    case 36:
      return "TL_INVALID_OPERATION: The operation cannot be completed in the current state.";
    case 40:
      return "TL_DISCONNECTING: The operation cannot be completed because the device is disconnecting.";
    case 41:
      return "TL_FIRMWARE_BUG: The device firmware reported an internal error.";
    case 42:
      return "TL_INITIALIZATION_FAILURE: The device failed to initialize.";
    case 43:
      return "TL_INVALID_CHANNEL: An invalid channel address was supplied.";

    // Motor-specific errors.
    case 37:
      return "TL_UNHOMED: The device must be homed before performing this operation.";
    case 38:
      return "TL_INVALID_POSITION: The requested operation would result in an illegal position.";
    case 39:
      return "TL_INVALID_VELOCITY_PARAMETER: The velocity parameter is invalid; velocity must be greater than zero.";
    case 44:
      return "TL_CANNOT_HOME_DEVICE: The device cannot perform homing. Check the limit-switch configuration.";
    case 45:
      return "TL_JOG_CONTINOUS_MODE: The configured jog mode is invalid for this jog operation.";
    case 46:
      return "TL_NO_MOTOR_INFO: Motor parameters required for real-world unit conversion are unavailable.";
    case 47:
      return "TL_CMD_TEMP_UNAVAILABLE: The command is temporarily unavailable; the device may be busy.";

    default:
      return "Unknown Kinesis error code: " + std::to_string(error_code);
  }
}

} // namespace


DeviceStatus::DeviceStatus(DeviceStatusCode error_code, std::optional<short> kinesis_error_code, std::string error_message)
            : error_code_ {error_code}, 
              kinesis_error_code_ {kinesis_error_code}, 
              error_message_{error_message} {}


DeviceStatus DeviceStatus::Ok()
{
    return DeviceStatus(DeviceStatusCode::kOk, 
                        std::nullopt, 
                        "Success");
}


DeviceStatus DeviceStatus::FromKinesis(short kinesis_error_code)
{
    if (kinesis_error_code == 0) {
        return Ok();
    }

    return DeviceStatus(DeviceStatusCode::kKinesisError, 
                        std::optional<short>{kinesis_error_code},
                        KinesisErrorMessage(kinesis_error_code));
}


DeviceStatus DeviceStatus::DeviceNotFound(const std::string& serial_number)
{
    return DeviceStatus(DeviceStatusCode::kDeviceNotFound,
                        std::nullopt,
                        "Device with serial number " + serial_number + " not found");
}

DeviceStatus DeviceStatus::FailedToLoadSettings(const std::string& serial_number);
{
    return DeviceStatus(DeviceStatusCode::kLoadSettingsError,
                        std::nullopt,
                        "Failed to load settings for device with serial number " + serial_number);
}

DeviceStatus DeviceStatus::FailedToStartPolling(const std::string& serial_number);
{
    return DeviceStatus(DeviceStatusCode::kPollingError,
                        std::nullopt,
                        "Failed to start polling device with serial number " + serial_number);
}
DeviceStatus DeviceStatus::NotConnected(const std::string& serial_number)
{
    return DeviceStatus(DeviceStatusCode::kConnectionError,
                        std::nullopt,
                        "Device with serial number " + serial_number + " not connected");
}

DeviceStatus DeviceStatus::Timeout(const std::string& serial_number, int expected_message)
{
    return DeviceStatus(DeviceStatusCode::kTimeout,
                        std::nullopt,
                        "Device with serial number " + serial_number + " timed out");
}

} // namespace thorlabs

