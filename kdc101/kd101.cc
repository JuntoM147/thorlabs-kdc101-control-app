#include "kdc101/kdc101.h"

#include "device_error/device_error.h"

namespace thorlabs {

constexpr int kDeviceID = 27; // KDC101 Device ID (from Thorlabs Kinesis C API)

namespace {

DeviceError find_device(const char* serialNo)
{   
    DeviceError rc(TLI_BuildDeviceList());
    if (!rc.Ok())
    {
        return rc;
    }

    // get KDC serial numbers
    char serialNumbers[250];
    TLI_GetDeviceListByTypeExt(serialNumbers, 250, kDeviceID);     // Get the device list which are dc servos

    // output list of matching devices
    char *searchContext = NULL;
    char *p = strtok_s(serialNumbers, ",", &searchContext);

    int found = 0;

    while (p != NULL)
    {
        TLI_DeviceInfo deviceInfo;

        // get device info from device
        TLI_GetDeviceInfo(p, &deviceInfo);

        // get strings from device info structure
        char curr_desc[65];
        strncpy_s(curr_desc, deviceInfo.description, 64);
        curr_desc[64] = '\0';

        char curr_serialNo[9];
        strncpy_s(curr_serialNo, deviceInfo.serialNo, 8);
        curr_serialNo[8] = '\0';

        if (strncmp(curr_serialNo, serialNo, 8) == 0) {
            found = 1;
        }

        // output
        p = strtok_s(NULL, ",", &searchContext);
    }

    return found;
}

int wait_for_motor_message(const char* serial_number_.c_str(), WORD expectedMessageId)
{
    WORD messageType = 0;
    WORD messageId = 0;
    DWORD messageData = 0;

    do {
        if (!CC_WaitForMessage(serial_number_.c_str(), &messageType, &messageId, &messageData)) {
            return 1;
        }

    } while (messageType != MESSAGE_TYPE_GENERIC_MOTOR || messageId != expectedMessageId);

    return 0;
}

void initialize_simulation()
{
    TLI_InitializeSimulations();
}

void uninitialize_simulation()
{
    TLI_UninitializeSimulations();
}

} // namespace


KDC101::KDC101(std::string serial_number, int polling_rate = kDefaultPollingRate, bool simulation_status = kDefaultSimulationStatus) 
    : serial_number_{serial_number},
    is_simulation_{simulation_status}
{
    int rc = find_device(serial_number_.c_str());
    if (rc == 0) {
        return 1; // Device not found
    } else if (rc == -1) {
        return 2; // Error occurred
    }

    rc = initialize(serial_number_.c_str());
    if (rc != 0) {
        return 1;
    }

    return 0;
}


KDC101::~KDC101()
{
    CC_ClearMessageQueue(serial_number.c_str());
    int rc = 0;

    // Stop the motor
    if (CC_StopProfiled(serial_number_.c_str()) != 0 ||
        wait_for_motor_message(serial_number_.c_str(), MESSAGE_ID_STOPPED) != 0) {
        rc = 1;
    }

    // Stop polling and close device
    CC_StopPolling(serial_number_.c_str());
    CC_DisableChannel(serial_number_.c_str()); // Disable the channel before closing
    CC_Close(serial_number_.c_str());

    return rc;


KDC101::Home(double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration)
{ 
    // Home device
    CC_ClearMessageQueue(serial_number_.c_str());
    
    if (CC_Home(serial_number_.c_str()) != 0) {
        return 1;
    }

    return wait_for_motor_message(serial_number_.c_str(), MESSAGE_ID_HOMED);
}


KDC101::Jog(Direction dir, double step_size = kDefaultStepSize, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration)
{
}


KDC101::StartDrive(Direction dir, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration)
{
}


KDC101::StopDrive()
{
}


KDC101::MoveAbsolute(double position, double speed = kDefaultSpeed, double acceleration = kDefaultAcceleration)
{
}


KDC101::Abort()
{
}

} // namespace thorlabs

