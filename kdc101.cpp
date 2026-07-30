#define DEVICE_ID 27
#define SERIAL_NUMBER 27000001
#define SERIAL_NUMBER_STR "27000001"

#include <stdlib.h>
#include <conio.h>
#include <stdio.h>

#include "Thorlabs.MotionControl.KCube.DCServo.h"


// Find the device with given serial number found -> 1 not found -> 0
int find_device(int serialNum) {
    if (TLI_BuildDeviceList() != 0)
    {
        printf("Failed to build device list\r\n");
        return 1;
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


int initialize(char* testSerialNo) {
    if(CC_Open(testSerialNo) != 0)
    {
        printf("Failed to open device with serial number %s\r\n", testSerialNo);
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


int move_position(const char *serialNo, int position)
{
    // move to position (channel 1)
    CC_ClearMessageQueue(serialNo);
    CC_MoveToPosition(serialNo, position);
    printf("Device %s moving\r\n", serialNo);

    // wait for completion
    WORD messageType;
    WORD messageId;
    DWORD messageData;

    do {
        CC_WaitForMessage(serialNo, &messageType, &messageId, &messageData);
        printf("Recieved message %hu with ID %hu and data %lu\r\n", messageType, messageId, messageData);
    } while(messageType != 2 || messageId != 0);

    printf("Device moved to position %d\r\n", position);

    return 0;
}

int get_position(const char *serialNo)
{
    // get actual position
    int pos = CC_GetPosition(serialNo);
    printf("Device %s moved to %d\r\n", serialNo, pos);

    return 0;
}


int wmain(int argc, wchar_t* argv[]) // wmain is for windows, same with wchar_t its wide char for windows
{
    TLI_InitializeSimulations();

    // Find device
    if(find_device(SERIAL_NUMBER) == 0) {
        printf("Failed to find device with serial number %d\r\n", SERIAL_NUMBER);
        TLI_UninitializeSimulations();
        return 1;
    } else {
        printf("Found device with serial number %d\r\n", SERIAL_NUMBER);
    }

    char* testSerialNo = SERIAL_NUMBER_STR;

    // Initialize device
    if (initialize(testSerialNo) != 0) {
        printf("Failed to initialize device\r\n");
        TLI_UninitializeSimulations();
        return 1;
    } else {
        printf("Initialized device\r\n");
        Sleep(3000); // Wait for 1 second to ensure the device is ready
    }

    // Home device
    home_device(testSerialNo);

    //Sleep(20000); // Wait for 3 seconds to ensure the device is homed

    // Move device to position 10
    // int position = 30;
    // move_position(testSerialNo, position);

    // get_position(testSerialNo);

    // Stop polling and close device
    CC_StopPolling(testSerialNo);
    CC_DisableChannel(testSerialNo); // Disable the channel before closing
    CC_Close(testSerialNo);
    TLI_UninitializeSimulations();

    return 0;
}
