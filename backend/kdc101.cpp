#include "kdc101.hpp"

#include <stdlib.h>
#include <conio.h>
#include <stdio.h>

#include "Thorlabs.MotionControl.KCube.DCServo.h"

#define DEVICE_ID 27
#define UNIT_TYPE_DISTANCE 0
#define UNIT_TYPE_VELOCITY 1
#define UNIT_TYPE_ACCELERATION 2
#define MESSAGE_TYPE_GENERIC_MOTOR 2
#define MESSAGE_ID_MOVED           1
#define MESSAGE_ID_STOPPED         2
#define MESSAGE_ID_HOMED           0

// Helper
int wait_for_motor_message(const char* serialNo, WORD expectedMessageId)
{
    WORD messageType = 0;
    WORD messageId = 0;
    DWORD messageData = 0;

    do {
        if (!CC_WaitForMessage(serialNo, &messageType, &messageId, &messageData)) {
            return 1;
        }

    } while (messageType != MESSAGE_TYPE_GENERIC_MOTOR || messageId != expectedMessageId);

    return 0;
}


void initialize_simulation(void)
{
    TLI_InitializeSimulations();
}

void uninitialize_simulation(void)
{
    TLI_UninitializeSimulations();
}

// 1 = found, 0 = not found, -1 = error
int find_device(const char* serialNo)
{   
    short rc = TLI_BuildDeviceList();
    if (rc != 0)
    {
        return -1;
    }

    // get KDC serial numbers
    char serialNumbers[250];
    TLI_GetDeviceListByTypeExt(serialNumbers, 250, DEVICE_ID);     // Get the device list which are dc servos

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


int initialize(const char* serialNo)
{
    if(CC_Open(serialNo) != 0) {
        return 1;
    }

    if (!CC_LoadSettings(serialNo)) {
        CC_Close(serialNo);
        return 1;
    }

    if (CC_EnableChannel(serialNo) != 0) {
        return 1;
    }

    // start the device polling at 200ms intervals
    CC_StartPolling(serialNo, 200);
    
    return 0;
}


int connect_device(const char* serialNo)
{
    int rc = find_device(serialNo);
    if (rc == 0) {
        return 1; // Device not found
    } else if (rc == -1) {
        return 2; // Error occurred
    }

    rc = initialize(serialNo);
    if (rc != 0) {
        return 1;
    }

    return 0;
}


int home_device(const char* serialNo)
{
    // Home device
    CC_ClearMessageQueue(serialNo);
    
    if (CC_Home(serialNo) != 0) {
        return 1;
    }

    return wait_for_motor_message(serialNo, MESSAGE_ID_HOMED);
}


int move_position(const char* serialNo, double position)
{
    // move to position (channel 1)
    CC_ClearMessageQueue(serialNo);

    // convert to device units
    int device_unit;
    if (CC_GetDeviceUnitFromRealValue(serialNo, position, &device_unit, UNIT_TYPE_DISTANCE) != 0) {
        return 1;
    }

    short rc = CC_MoveToPosition(serialNo, device_unit);
    if (rc != 0) {
        return 1;
    }

    return wait_for_motor_message(serialNo, MESSAGE_ID_MOVED);
}


double get_position(const char* serialNo)
{
    int device_unit = CC_GetPosition(serialNo);
    double position_real;
    CC_GetRealValueFromDeviceUnit(serialNo, device_unit, &position_real, UNIT_TYPE_DISTANCE);

    return position_real;
}


int jog(const char* serialNo, int direction)
{
    CC_ClearMessageQueue(serialNo);

    short rc = CC_SetJogMode(serialNo, MOT_JogModes::MOT_SingleStep, MOT_StopModes::MOT_Profiled);
    if (rc != 0) {
        return 1;
    }

    MOT_TravelDirection mot_direction;

    if (direction == 1) {
        mot_direction = MOT_TravelDirection::MOT_Forwards;
    } else {
        mot_direction = MOT_TravelDirection::MOT_Backwards;
    }

    rc = CC_MoveJog(serialNo, mot_direction);
    if (rc != 0) {
        return 1;
    }

    if (wait_for_motor_message(serialNo, MESSAGE_ID_MOVED) != 0) {
        return 1;
    }

    return 0;
}


int start_drive(const char* serialNo, int direction)
{
    MOT_TravelDirection travelDirection;

    if (direction == 1) {
        travelDirection = MOT_TravelDirection::MOT_Forwards;
    } else if (direction == -1) {
        travelDirection = MOT_TravelDirection::MOT_Backwards;
    } else {
        return 1;
    }

    return CC_MoveAtVelocity(serialNo, travelDirection) == 0 ? 0 : 1;
}


int stop_drive(const char* serialNo)
{
    return CC_StopProfiled(serialNo) == 0 ? 0 : 1;
}


int close_device(const char* serialNo)
{
    CC_ClearMessageQueue(serialNo);

    int rc = 0;

    // Stop the motor
    if (CC_StopProfiled(serialNo) != 0 ||
        wait_for_motor_message(serialNo, MESSAGE_ID_STOPPED) != 0) {
        rc = 1;
    }

    // Stop polling and close device
    CC_StopPolling(serialNo);
    CC_DisableChannel(serialNo); // Disable the channel before closing
    CC_Close(serialNo);

    return rc;
}
