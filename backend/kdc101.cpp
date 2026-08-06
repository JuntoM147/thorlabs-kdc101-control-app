#include "kdc101.hpp"

#include <stdlib.h>
#include <conio.h>
#include <stdio.h>

#include "Thorlabs.MotionControl.KCube.DCServo.h"

#define DEVICE_ID 27
#define SERIAL_NUMBER 27000001
#define SERIAL_NUMBER_STR "27000001"

#define UNIT_TYPE_DISTANCE 0
#define UNIT_TYPE_VELOCITY 1
#define UNIT_TYPE_ACCELERATION 2
#define MESSAGE_TYPE_GENERIC_MOTOR 2
#define MESSAGE_ID_MOVED           1
#define MESSAGE_ID_STOPPED         2
#define MESSAGE_ID_HOMED           0

constexpr char const *serialNo = SERIAL_NUMBER_STR;

// Helper
int wait_for_motor_message(WORD expectedMessageId)
{
    WORD messageType = 0;
    WORD messageId = 0;
    DWORD messageData = 0;

    do {
        if (!CC_WaitForMessage(serialNo, &messageType, &messageId, &messageData)) {
            printf("Failed while waiting for device message\r\n");
            return 1;
        }

        printf("Received message type %hu, ID %hu, data %lu\r\n", messageType, messageId, (unsigned long)messageData);

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
int find_device(void) 
{   
    short rc = TLI_BuildDeviceList();
    if (rc != 0)
    {
        printf("Failed to build device list %hd\r\n", rc);
        return -1;
    }

    // get device list size   
    short n = TLI_GetDeviceListSize();   

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
        printf("Found Device! Serial Number: %s : %s\r\n", curr_serialNo, curr_desc);
        p = strtok_s(NULL, ",", &searchContext);
    }

    return found;
}


int initialize(void) 
{
    if(CC_Open(serialNo) != 0)
    {
        printf("Failed to open device with serial number %s\r\n", serialNo);
        return 1;
    }

    if (!CC_LoadSettings(serialNo)) {
        printf("Failed to load device settings\r\n");
        CC_Close(serialNo);
        return 1;
    }

    // Use the enable function from your SDK version
    if (CC_EnableChannel(serialNo) != 0) { 
        printf("Failed to enable device channel\r\n");
        return 1;
    }

    // start the device polling at 200ms intervals
    CC_StartPolling(serialNo, 200);
    
    return 0;
}


int home_device(void)
{
    // Home device
    CC_ClearMessageQueue(serialNo);
    
    CC_Home(serialNo);
    printf("Device %s homing\r\n", serialNo);

    wait_for_motor_message(MESSAGE_ID_HOMED); // Wait for homing complete message

    printf("Device homed\r\n");

    return 0;
}


int move_position(const double position)
{
    // move to position (channel 1)
    CC_ClearMessageQueue(serialNo);

    // convert to device units
    int device_unit;
    CC_GetDeviceUnitFromRealValue(serialNo, position, &device_unit, UNIT_TYPE_DISTANCE);
    printf("Moving device %s to position %.2f mm (device units: %d)\r\n", serialNo, position, device_unit);

    short rc = CC_MoveToPosition(serialNo, device_unit);
    if (rc != 0) {
        printf("Failed to start move: %hd\r\n", rc);
        return 1;
    }
    
    printf("Device %s moving\r\n", serialNo);

    wait_for_motor_message(MESSAGE_ID_MOVED);

    printf("Device done moving to position %g mm\r\n", position);

    return 0;
}


double get_position(void)
{
    int device_unit = CC_GetPosition(serialNo);
    double position_real;
    CC_GetRealValueFromDeviceUnit(serialNo, device_unit, &position_real, UNIT_TYPE_DISTANCE);

    return position_real;
}


int move_relative(const double displacement)
{
    CC_ClearMessageQueue(serialNo);

    // convert to device units
    int device_unit;
    CC_GetDeviceUnitFromRealValue(serialNo, displacement, &device_unit, UNIT_TYPE_DISTANCE);
    printf("Moving device %s by displacement %.2f mm (device units: %d)\r\n", serialNo, displacement, device_unit);

    short rc = CC_MoveRelative(serialNo, device_unit);
    if (rc != 0) {
        printf("Failed to start relative move: %hd\r\n", rc);
        return 1;
    }
    
    printf("Device %s moving\r\n", serialNo);

    wait_for_motor_message(MESSAGE_ID_MOVED);

    printf("Device done moving by displacement %g mm\r\n", displacement);

    return 0;
}


int jog(JogDirection direction)
{
    CC_ClearMessageQueue(serialNo);

    short rc = CC_SetJogMode(serialNo, MOT_JogModes::MOT_SingleStep, MOT_StopModes::MOT_Profiled);
    if (rc != 0) {
        printf("Failed to set jog mode: %hd\r\n", rc);
        return 1;
    }

    MOT_TravelDirection mot_direction;

    if (direction == JogDirection::Forwards) {
        mot_direction = MOT_TravelDirection::MOT_Forwards;
    } else {
        mot_direction = MOT_TravelDirection::MOT_Backwards;
    }

    rc = CC_MoveJog(serialNo, mot_direction);
    if (rc != 0) {
        printf("Failed to start jog: %hd\r\n", rc);
        return 1;
    }

    printf("Device %s jogging in direction %d for a single step\r\n", serialNo, direction);
    if (wait_for_motor_message(MESSAGE_ID_MOVED) != 0) {
        return 1;
    }
    
    printf("Device %s stopped jogging\r\n", serialNo);

    return 0;
}


int close(void)
{
    // Stop polling and close device
    CC_StopPolling(serialNo);
    CC_DisableChannel(serialNo); // Disable the channel before closing
    CC_Close(serialNo);
    TLI_UninitializeSimulations();

    return 0;
}
