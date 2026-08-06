#include "kdc101.hpp"

#include <stdio.h>

int wmain(int argc, wchar_t* argv[]) // wmain is for windows, same with wchar_t its wide char for windows
{
    initialize_simulation();
    
    // Find device
    int rc = find_device();
    if(rc < 0) {
        printf("Error occurred while searching for device\r\n");
        uninitialize_simulation();
        return 1;
    } else if(rc == 0) {
        printf("Device not found\r\n");
        uninitialize_simulation();
        return 1;
    } else {
        printf("Found device\r\n");
    }

    // Initialize device
    if (initialize() != 0) {
        printf("Failed to initialize device\r\n");
        uninitialize_simulation();
        return 1;
    } else {
        printf("Initialized device\r\n");
    }

    // Home device
    home_device();

    // Move device to position 30
    const double position = 10.0; // Target in real units
    move_position(position);

    printf("Get position: Device is at %g mm \r\n", get_position());
    
    const double displacement = 5.0; // Displacement in real units
    move_relative(displacement);

    printf("Get position: Device is at %g mm \r\n", get_position());

    jog(JogDirection::Forwards);

    printf("Get position: Device is at %g mm \r\n", get_position());

    close();

    return 0;
}
