#include "kdc101/kdc101.h"

#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <expected>
#include <thread>
#include <array>
#include <ranges>
#include <sstream>
#include <utility>

#include "device_status/device_status.h"
#include "Thorlabs.MotionControl.KCube.DCServo.h"


namespace thorlabs {

constexpr int kDeviceID = 27; // KDC101 Device ID (from Thorlabs Kinesis C API)
constexpr size_t kDeviceListBufferSize = 250;
constexpr char kStageID[] = "Z825";

namespace {

DeviceStatus GetDevices(std::vector<std::string>& serial_numbers)
{
    // Build list of connected devices
    DeviceStatus build_list_status = DeviceStatus::FromKinesis(TLI_BuildDeviceList());
    if (!build_list_status.ok()) { 
        return build_list_status;
    }

    // get LTS serial numbers
    std::array<char, kDeviceListBufferSize> device_list_buffer {};
    DeviceStatus device_list_status = DeviceStatus::FromKinesis(TLI_GetDeviceListByTypeExt(device_list_buffer.data(), 
                                                                static_cast<DWORD>(device_list_buffer.size()), 
                                                                kDeviceID));
    if (!device_list_status.ok()) { 
        return device_list_status; 
    }

    // populate vector with the serial numbers found
    std::string device_list(device_list_buffer.data());
    std::stringstream ss(device_list);
    std::string curr_serial_no; 

    while (std::getline(ss, curr_serial_no, ',')) {
        serial_numbers.push_back(curr_serial_no);
    }

    return DeviceStatus::Ok();
}


DeviceStatus FindDevice(const std::string& serial_number)
{   
    std::vector<std::string> serial_numbers {};

    DeviceStatus device_list_status = GetDevices(serial_numbers);
    if (!device_list_status.ok()) { 
        return device_list_status; 
    };

    const auto iter = std::ranges::find(serial_numbers, serial_number);

    if (iter == serial_numbers.end()) { 
        return DeviceStatus::DeviceNotFound(serial_number); 
    }

    return DeviceStatus::Ok();
}


DeviceStatus OpenDevice(const std::string& serial_number)
{
    DeviceStatus open_status = DeviceStatus::FromKinesis(CC_Open(serial_number.c_str()));
    if (!open_status.ok()) { return open_status; }

    return DeviceStatus::Ok();
}


DeviceStatus LoadSettings(const std::string& serial_number)
{
    if (!CC_LoadSettings(serial_number.c_str()) && !CC_LoadNamedSettings(serial_number.c_str(), kStageID)) {
        return DeviceStatus::FailedToLoadSettings(serial_number);
    }

    return DeviceStatus::Ok();
}


DeviceStatus EnableChannel(const std::string& serial_number)
{
    DeviceStatus channel_status = DeviceStatus::FromKinesis(CC_EnableChannel(serial_number.c_str()));
    if (!channel_status.ok()) { return channel_status; }

    return DeviceStatus::Ok();
}


DeviceStatus StartPolling(const std::string& serial_number, int polling_interval_ms)
{
    // Start the device polling 
    if (!CC_StartPolling(serial_number.c_str(), polling_interval_ms)) {
        return DeviceStatus::FailedToStartPolling(serial_number);
    }

    return DeviceStatus::Ok();
}

} // namespace


KDC101::KDC101(std::string serial_number, bool simulation_status) 
    : serial_number_{serial_number},
      simulation_{simulation_status},
      connected_{false},
      polling_{false},
      channel_enabled_{false} {}


KDC101::CreateResult KDC101::Create(std::string serial_number, int polling_interval_ms, bool simulation)
{
    auto device = std::unique_ptr<KDC101>(
    new KDC101(serial_number, simulation));

    if(device->simulation_) {
        TLI_InitializeSimulations();
    }

    // Find device
    DeviceStatus status = FindDevice(serial_number);
    if (!status.ok()) {
        return std::unexpected(status);
    }

    // Open device
    status = OpenDevice(serial_number);
    if (!status.ok()) {
        return std::unexpected(status);
    }

    device->connected_ = true;

    // Load Settings
    status = LoadSettings(serial_number);
    if (!status.ok()) {
        return std::unexpected(status);
    }

    // Enable Channel
    status = EnableChannel(serial_number);
    if (!status.ok()) {
        return std::unexpected(status);
    }

    device->channel_enabled_ = true;

    // Start Polling
    status = StartPolling(serial_number, polling_interval_ms);
    if (!status.ok()) {
        return std::unexpected(status);
    }

    device->polling_ = true;

    return device;
}


KDC101::~KDC101()
{
    if (connected_) {
        CC_ClearMessageQueue(serial_number_.c_str());
        CC_StopProfiled(serial_number_.c_str());    // No point waiting for stop to finish
    }

    if (channel_enabled_) {
        CC_DisableChannel(serial_number_.c_str());
    }

    if (polling_) {
        CC_StopPolling(serial_number_.c_str());
    }

    if (connected_) {
        CC_Close(serial_number_.c_str());
    }

    if (simulation_) {
        TLI_UninitializeSimulations();
      }
}


namespace {

constexpr int kUnitTypeDistance = 0;
constexpr int kUnitTypeSpeed = 1;
constexpr int kUnitTypeAcceleration = 2;

}

KDC101::PositionResult KDC101::GetPosition()
{
    int device_unit = CC_GetPosition(serial_number_.c_str());

    double position_real;
    DeviceStatus conversion_status = DeviceStatus::FromKinesis(CC_GetRealValueFromDeviceUnit(serial_number_.c_str(), device_unit, &position_real, kUnitTypeDistance); 

    if (!conversion_status.ok()) {
        return std::unexpected(conversion_status);
    }

    return position_real;
}


DeviceStatus KDC101::Home(double speed, double acceleration)
{ 
    CC_ClearMessageQueue(serial_number_.c_str());
    
    DeviceStatus home_status = DeviceStatus::FromKinesis(CC_Home(serial_number_.c_str()));

    if (!home_status.ok()) { 
        return home_status;
    }

    return WaitForMotorMessage(kMessageIdHomed);
}


DeviceStatus KDC101::Jog(Direction dir, double step_size, double speed, double acceleration)
{
    CC_ClearMessageQueue(serial_number_.c_str());

    int device_unit;
    DeviceStatus conversion_status = DeviceStatus::FromKinesis(CC_GetDeviceUnitFromRealValue(serial_number_.c_str()
                                                                                             position, 
                                                                                             &device_unit, 
                                                                                             kUnitTypeDistance));

    if (!conversion_status.ok()) {
        return conversion_status;
    }

    DeviceStatus jog_mode_status = DeviceStatus::FromKinesis(CC_SetJogMode(serial_number_.c_str(), MOT_JogModes::MOT_SingleStep, MOT_StopModes::MOT_Profiled));
    if (!jog_mode_status.ok()) {
        return jog_mode_status;
    }

    DeviceStatus set_step_size_status = DeviceStatus::FromKinesis(CC_SetJogStepSize(serialNo, static_cast<unsigned int>(device_unit)));
    if (!step_size_status.ok()) {
        return set_step_size_status;
    }

    MOT_TravelDirection mot_direction;

    if (dir == Direction::kForward) {
        mot_direction = MOT_TravelDirection::MOT_Forwards;
    } else {
        mot_direction = MOT_TravelDirection::MOT_Backwards;
    } 

    DeviceStatus move_status = DeviceStatus::FromKinesis(CC_MoveJog(serial_number_.c_str(), mot_direction));
    if (!move_status.ok()) {
        return move_status;
    }

    return WaitForMotorMessage(kMessageIdMoved);
}


DeviceStatus KDC101::StartDrive(Direction dir, double speed, double acceleration)
{
    MOT_TravelDirection mot_direction;

    if (dir == Direction::kForward) {
        mot_direction = MOT_TravelDirection::MOT_Forwards;
    } else {
        mot_direction = MOT_TravelDirection::MOT_Backwards;
    } 

     return DeviceStatus::FromKinesis(CC_MoveAtVelocity(serial_number_.c_str(), mot_direction));
}


DeviceStatus KDC101::Stop(StopMode stop_mode)
{
    if (stop_mode == StopMode::kProfiled) {
        return DeviceStatus::FromKinesis(CC_StopProfiled(serial_number_.c_str()));
    } 

    return DeviceStatus::FromKinesis(CC_StopImmediate(serial_number_.c_str()));
}


DeviceStatus KDC101::MoveAbsolute(double position, double speed, double acceleration)
{
    CC_ClearMessageQueue(serial_number_.c_str());

    // convert to device units
    int device_unit;
    DeviceStatus conversion_status = DeviceStatus::FromKinesis(CC_GetDeviceUnitFromRealValue(serial_number_.c_str(),
                                                                                             position, 
                                                                                             &device_unit, 
                                                                                             kUnitTypeDistance));

    if (!conversion_status.ok()) {
        return conversion_status;
    }

    DeviceStatus move_status = DeviceStatus::FromKinesis(CC_MoveToPosition(serial_number_.c_str(), device_unit));
    if (!move_status.ok()) {
        return move_status;
    }

    return WaitForMotorMessage(kMessageIdMoved);
}


namespace {

constexpr WORD kMessageTypeGenericMotor = 0;
constexpr WORD kMessageIdHomed = 0;
constexpr WORD kMessageIdMoved = 1;
constexpr WORD kMessageIdStopped = 2;
constexpr auto kMessageCheckInterval = std::chrono::milliseconds(10);

} // namespace


DeviceStatus KDC101::WaitForMotorMessage(int expected_message, std::chrono::milliseconds timeout_ms)
{
    const WORD expected_message_id = static_cast<WORD>(expected_message);
    const auto deadline = std::chrono::steady_clock::now() + timeout_ms;

    while (std::chrono::steady_clock::now() < deadline) {
        WORD message_type = 0;
        WORD message_id = 0;
        DWORD message_data = 0;

        // CC_GetNextMessage over CC_WaitForMessage because CC_WaitForMessage blocks
        while (CC_GetNextMessage(serial_number_.c_str(), &message_type, &message_id, &message_data)) {
            if (message_type == kMessageTypeGenericMotor && message_id == expected_message_id) {
                return DeviceStatus::Ok();
            }
        }

        if (!CC_CheckConnection(serial_number_.c_str())) {
            connected_ = false;
            return DeviceStatus::NotConnected();
        }

        std::this_thread::sleep_for(kMessageCheckInterval);
    }

    return DeviceStatus::Timeout(serial_number_, expected_message_id);
}


} // namespace thorlabs

