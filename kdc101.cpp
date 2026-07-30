#define DEVICE_ID 27
#define SERIAL_NUMBER 27000001
#define SERIAL_NUMBER_STR "27000001"

#define UNIT_TYPE_DISTANCE 0
#define UNIT_TYPE_VELOCITY 1
#define UNIT_TYPE_ACCELERATION 2

#include <stdlib.h>
#include <conio.h>
#include <stdio.h>

#include "Thorlabs.MotionControl.KCube.DCServo.h"


// 1 = found, 0 = not found, -1 = error
int find_device(void) {
    if (TLI_BuildDeviceList() != 0)
    {
        printf("Failed to build device list\r\n");
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

        if (strncmp(curr_serialNo, SERIAL_NUMBER_STR, 8) == 0) {
            found = 1;
        }

        // output
        printf("Found Device! Serial Number: %s : %s\r\n", curr_serialNo, curr_desc);
        p = strtok_s(NULL, ",", &searchContext);
    }

    return found;
}


int initialize(const char* testSerialNo) {
    if(CC_Open(testSerialNo) != 0)
    {
        printf("Failed to open device with serial number %s\r\n", testSerialNo);
        return 1;
    }

    if (!CC_LoadSettings(testSerialNo)) {
        printf("Failed to load device settings\r\n");
        CC_Close(testSerialNo);
        return 1;
    }

    // Use the enable function from your SDK version
    if (CC_EnableChannel(testSerialNo) != 0) { 
        printf("Failed to enable device channel\r\n");
        return 1;
    }

    // start the device polling at 200ms intervals
    CC_StartPolling(testSerialNo, 200);
    
    return 0;
}


int home_device(const char *serialNo)
{
    // Home device
    CC_ClearMessageQueue(serialNo);
    CC_Home(serialNo);
    printf("Device %s homing\r\n", serialNo);

    // wait for completion
    WORD messageType;
    WORD messageId;
    DWORD messageData;

    do {
        CC_WaitForMessage(serialNo, &messageType, &messageId, &messageData);
        printf("Recieved message %hu with ID %hu and data %lu\r\n", messageType, messageId, messageData);
    } while(messageType != 2 || messageId != 0);

    printf("Device homed\r\n");

    return 0;
}


int move_position(const char *serialNo, const double position)
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

    // wait for completion
    WORD messageType;
    WORD messageId;
    DWORD messageData;

    do {
        CC_WaitForMessage(serialNo, &messageType, &messageId, &messageData);
        printf("Recieved message %hu with ID %hu and data %lu\r\n", messageType, messageId, messageData);
    } while(messageType != 2 || messageId != 1);

    printf("Device done moving to position %g mm\r\n", position);

    return 0;
}

int get_position(const char *serialNo)
{
    int device_unit = CC_GetPosition(serialNo);
    double position_real;
    CC_GetRealValueFromDeviceUnit(serialNo, device_unit, &position_real, UNIT_TYPE_DISTANCE);

    printf("Get position: Device %s is at %g mm (device units: %d)\r\n", serialNo, position_real, device_unit);

    return 0;
}


int wmain(int argc, wchar_t* argv[]) // wmain is for windows, same with wchar_t its wide char for windows
{
    TLI_InitializeSimulations();

    // Find device
    int rc = find_device();
    if(rc < 0) {
        printf("Error occurred while searching for device with serial number %d\r\n", SERIAL_NUMBER);
        TLI_UninitializeSimulations();
        return 1;
    } else if(rc == 0) {
        printf("Device with serial number %d not found\r\n", SERIAL_NUMBER);
        TLI_UninitializeSimulations();
        return 1;
    } else {
        printf("Found device with serial number %d\r\n", SERIAL_NUMBER);
    }

    const char* testSerialNo = SERIAL_NUMBER_STR;

    // Initialize device
    if (initialize(testSerialNo) != 0) {
        printf("Failed to initialize device\r\n");
        TLI_UninitializeSimulations();
        return 1;
    } else {
        printf("Initialized device\r\n");
    }

    // Home device
    home_device(testSerialNo);

    // Move device to position 30
    const double position = 10.0; // Target in real units
    move_position(testSerialNo, position);

    get_position(testSerialNo);

    // Stop polling and close device
    CC_StopPolling(testSerialNo);
    CC_DisableChannel(testSerialNo); // Disable the channel before closing
    CC_Close(testSerialNo);
    TLI_UninitializeSimulations();

    return 0;
}
