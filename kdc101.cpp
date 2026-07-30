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

    // start the device polling at 200ms intervals
    CC_StartPolling(testSerialNo, 200);
    
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

    // Initialize device
    if (initialize(SERIAL_NUMBER_STR) != 0) {
        printf("Failed to initialize device\r\n");
        TLI_UninitializeSimulations();
        return 1;
    } else {
        printf("Initialized device\r\n");
    }

    TLI_UninitializeSimulations();

    return 0;
}
