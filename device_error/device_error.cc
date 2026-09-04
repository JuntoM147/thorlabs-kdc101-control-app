#include "device_error/device_error.h"

#include <string>
 

namespace thorlabs {

DeviceError::DeviceError(int error_code) : error_code_ {error_code}
{
    switch (error_code)
    {
        case 0:
            error_message_ = "Success.";
            break;


        // FTDI and communication errors
        case 1:
            error_message_ = "FT_InvalidHandle: The FTDI functions have not been initialized.";
            break;

        case 2:
            error_message_ = "FT_DeviceNotFound: The device could not be found. Ensure TLI_BuildDeviceList() has been called.";
            break;

        case 3:
            error_message_ = "FT_DeviceNotOpened: The device must be opened before it can be accessed.";
            break;

        case 4:
            error_message_ = "FT_IOError: An I/O error occurred in the FTDI communication interface.";
            break;

        case 5:
            error_message_ = "FT_InsufficientResources: Insufficient resources are available.";
            break;

        case 6:
            error_message_ = "FT_InvalidParameter: An invalid parameter was supplied.";
            break;

        case 7:
            error_message_ = "FT_DeviceNotPresent: The device is no longer connected.";
            break;

        case 8:
            error_message_ = "FT_IncorrectDevice: The detected device does not match the expected device.";
            break;


        // Device-library errors
        case 16:
            error_message_ = "FT_NoDLLLoaded: The required device library could not be found.";
            break;

        case 17:
            error_message_ = "FT_NoFunctionsAvailable: No functions are available for this device.";
            break;

        case 18:
            error_message_ = "FT_FunctionNotAvailable: The requested function is not available for this device.";
            break;

        case 19:
            error_message_ = "FT_BadFunctionPointer: A bad function pointer was detected.";
            break;

        case 20:
            error_message_ = "FT_GenericFunctionFail: The function failed to complete successfully.";
            break;

        case 21:
            error_message_ = "FT_SpecificFunctionFail: The function failed to complete successfully.";
            break;


        // General DLL control errors
        case 32:
            error_message_ = "TL_ALREADY_OPEN: The device is already open.";
            break;

        case 33:
            error_message_ = "TL_NO_RESPONSE: The device has stopped responding.";
            break;

        case 34:
            error_message_ = "TL_NOT_IMPLEMENTED: The requested function has not been implemented.";
            break;

        case 35:
            error_message_ = "TL_FAULT_REPORTED: The device reported a fault.";
            break;

        case 36:
            error_message_ = "TL_INVALID_OPERATION: The operation cannot be completed in the current state.";
            break;

        case 40:
            error_message_ = "TL_DISCONNECTING: The operation cannot be completed because the device is disconnecting.";
            break;

        case 41:
            error_message_ = "TL_FIRMWARE_BUG: The device firmware reported an internal error.";
            break;

        case 42:
            error_message_ = "TL_INITIALIZATION_FAILURE: The device failed to initialize.";
            break;

        case 43:
            error_message_ = "TL_INVALID_CHANNEL: An invalid channel address was supplied.";
            break;


        // Motor-specific errors
        case 37:
            error_message_ = "TL_UNHOMED: The device must be homed before performing this operation.";
            break;

        case 38:
            error_message_ = "TL_INVALID_POSITION: The requested operation would result in an illegal position.";
            break;

        case 39:
            error_message_ = "TL_INVALID_VELOCITY_PARAMETER: The velocity parameter is invalid; velocity must be greater than zero.";
            break;

        case 44:
            error_message_ = "TL_CANNOT_HOME_DEVICE: The device cannot perform homing. Check the limit-switch configuration.";
            break;

        case 45:
            error_message_ = "TL_JOG_CONTINOUS_MODE: The configured jog mode is invalid for this jog operation.";
            break;

        case 46:
            error_message_ = "TL_NO_MOTOR_INFO: Motor parameters required for real-world unit conversion are unavailable.";
            break;

        case 47:
            error_message_ = "TL_CMD_TEMP_UNAVAILABLE: The command is temporarily unavailable; the device may be busy.";
            break;


        default:
            error_message_ = "Unknown Kinesis error code: " + std::to_string(error_code);
            break;
    }
}

} // namespace thorlabs

