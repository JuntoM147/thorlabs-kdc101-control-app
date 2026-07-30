#define DEVICE_ID 27
#define SERIAL_NUMBER 26000001

#include <stdlib.h>
#include <conio.h>
#include <stdio.h>

#include "Thorlabs.MotionControl.KCube.DCServo.h"


// Find the device with given serial number
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
    printf("%s \n", serialNumbers);

    // output list of matching devices
    {
        char *searchContext = NULL;
        char *p = strtok_s(serialNumbers, ",", &searchContext);

        while (p != NULL)
        {
            TLI_DeviceInfo deviceInfo;

            // get device info from device
            TLI_GetDeviceInfo(p, &deviceInfo);

            // get strings from device info structure
            char desc[65];
            strncpy_s(desc, deviceInfo.description, 64);
            desc[64] = '\0';
            char serialNo[9];
            strncpy_s(serialNo, deviceInfo.serialNo, 8);
            serialNo[8] = '\0';

            // output
            printf("Found Device %s=%s : %s\r\n", p, serialNo, desc);
            p = strtok_s(NULL, ",", &searchContext);
        }
    }

    return 0;
}


int wmain(int argc, wchar_t* argv[]) // wmain is for windows, same with wchar_t its wide char for windows
{
    TLI_InitializeSimulations();

    // identify and access device
    find_device(SERIAL_NUMBER);

    char testSerialNo[16] = "SERIAL_NUMBER";
    sprintf_s(testSerialNo, "%d", SERIAL_NUMBER);

    TLI_UninitializeSimulations();

    return 0;
}
