#include "kdc101/kdc101.h"
#include "kinesis_simulation/kinesis_simulation.h"

#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <expected>
#include <array>
#include <ranges>
#include <sstream>
#include <utility>
#include <mutex>
#include <set>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <limits>

#include "device_status/device_status.h"
#include "Thorlabs.MotionControl.KCube.DCServo.h"


namespace thorlabs {

constexpr int kDeviceID = 27; // KDC101 Device ID (from Thorlabs Kinesis C API)
constexpr size_t kDeviceListBufferSize = 250;
constexpr char kStageID[] = "Z825";

namespace {

// Device enumeration and open/close mutate SDK-wide state across axis workers.
// Recursive only so a partially constructed device can clean up on failure.
std::recursive_mutex connection_mutex;
std::set<std::string> open_serials;

// Keep the SDK boundary observable in GUI builds, which have no console.
// Logging failure must never affect a motor command.
void TraceRelativeMove(const std::string& serial, double distance, int units,
                       int position_before, short result) noexcept {
    try {
        static std::mutex trace_mutex;
        std::lock_guard lock(trace_mutex);
        std::error_code error;
        const auto directory = std::filesystem::temp_directory_path(error);
        if (error) return;
        std::ofstream trace(directory / "confo-quanta-motion.log", std::ios::app);
        const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        trace << timestamp << " serial=" << serial << std::setprecision(17)
              << " relative_mm=" << distance << " device_units=" << units
              << " cached_position_before_units=" << position_before
              << " sdk_result=" << result << '\n';
    } catch (...) {
        // Diagnostics are best effort, including when the temp directory is unavailable.
    }
}

DeviceStatus GetDevices(std::vector<std::string>& serial_numbers)
{
    // Build list of connected devices
    DeviceStatus build_list_status = DeviceStatus::FromKinesis(TLI_BuildDeviceList());
    if (!build_list_status.ok()) {
        return build_list_status.WithContext("TLI_BuildDeviceList");
    }

    // get LTS serial numbers
    std::array<char, kDeviceListBufferSize> device_list_buffer {};
    DeviceStatus device_list_status = DeviceStatus::FromKinesis(TLI_GetDeviceListByTypeExt(device_list_buffer.data(),
                                                                static_cast<DWORD>(device_list_buffer.size()),
                                                                kDeviceID));
    if (!device_list_status.ok()) {
        return device_list_status.WithContext("TLI_GetDeviceListByTypeExt");
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
    if (!open_status.ok()) { return open_status.WithContext("CC_Open"); }

    return DeviceStatus::Ok();
}


DeviceStatus LoadSettings(const std::string& serial_number)
{
    if (!CC_LoadSettings(serial_number.c_str()) && !CC_LoadNamedSettings(serial_number.c_str(), kStageID)) {
        return DeviceStatus::FailedToLoadSettings(serial_number).WithContext("CC_LoadSettings / CC_LoadNamedSettings");
    }

    return DeviceStatus::Ok();
}


DeviceStatus EnableChannel(const std::string& serial_number)
{
    DeviceStatus channel_status = DeviceStatus::FromKinesis(CC_EnableChannel(serial_number.c_str()));
    if (!channel_status.ok()) { return channel_status.WithContext("CC_EnableChannel"); }

    return DeviceStatus::Ok();
}


DeviceStatus StartPolling(const std::string& serial_number, int polling_interval_ms)
{
    // Start the device polling
    if (!CC_StartPolling(serial_number.c_str(), polling_interval_ms)) {
        return DeviceStatus::FailedToStartPolling(serial_number).WithContext("CC_StartPolling");
    }

    return DeviceStatus::Ok();
}

} // namespace


KDC101::KDC101(std::string serial_number, std::shared_ptr<const KinesisSimulation> simulation)
    : serial_number_{serial_number},
      simulation_{std::move(simulation)},
      connected_{false},
      polling_{false},
      channel_enabled_{false} {}


KDC101::CreateResult KDC101::CreateMotor(std::string serial_number, int polling_interval_ms, std::shared_ptr<const KinesisSimulation> simulation)
{
    std::lock_guard lock(connection_mutex);
    if (open_serials.contains(serial_number)) {
        return std::unexpected(DeviceStatus::FromKinesis(32).WithContext(
            "Serial " + serial_number + " is already owned by another motor connection in this application"));
    }
    auto device = std::unique_ptr<KDC101>(new KDC101(serial_number, std::move(simulation)));

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
    open_serials.insert(serial_number);

    // Capture the controller before a profile can overwrite inherited settings.
    status = device->SaveSettingsSnapshot("before_profile");
    if (!status.ok()) return std::unexpected(status);

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

    status = device->SaveSettingsSnapshot("after_profile");
    if (!status.ok()) return std::unexpected(status);
    status = device->SetBacklash(kDefaultBacklashMm);
    if (!status.ok()) return std::unexpected(status.WithContext("Apply startup backlash"));
    status = device->SaveSettingsSnapshot("after_startup_backlash");
    if (!status.ok()) return std::unexpected(status);
    if (CC_GetBacklash(serial_number.c_str()) != 0) {
        return std::unexpected(DeviceStatus::FailedToLoadSettings(serial_number)
            .WithContext("Startup backlash readback did not match zero device units"));
    }

    return device;
}


KDC101::~KDC101()
{
    std::lock_guard lock(connection_mutex);
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
        open_serials.erase(serial_number_);
    }
}


namespace {

constexpr int kUnitTypeDistance = 0;
constexpr int kUnitTypeSpeed = 1;
constexpr int kUnitTypeAcceleration = 2;


std::expected<int, DeviceStatus> ConvertMotionParameter(const std::string& serial_number, double value, int unit_type) {
    if (!std::isfinite(value) || value <= 0.0) {
        return std::unexpected(DeviceStatus::FromKinesis(FT_InvalidParameter));
    }
    int device_units = 0;
    auto status = DeviceStatus::FromKinesis(CC_GetDeviceUnitFromRealValue(serial_number.c_str(), value, &device_units, unit_type));

    if (!status.ok()) {
        return std::unexpected(status);
    }

    if (device_units <= 0) {
        return std::unexpected(DeviceStatus::FromKinesis(FT_InvalidParameter));
    }

    return device_units;
}


DeviceStatus ApplyVelocity(const std::string& serial_number, VelocityParameters update, bool jog) {
    std::optional<int> speed, acceleration;
    if (update.speed_mm_per_second) {
        auto converted = ConvertMotionParameter(serial_number, *update.speed_mm_per_second, kUnitTypeSpeed);
        if (!converted) return converted.error();
        speed = *converted;
    }
    if (update.acceleration_mm_per_second_squared) {
        auto converted = ConvertMotionParameter(serial_number, *update.acceleration_mm_per_second_squared, kUnitTypeAcceleration);
        if (!converted) return converted.error();
        acceleration = *converted;
    }
    if (!speed && !acceleration) return DeviceStatus::Ok();
    int device_speed = 0, device_acceleration = 0;
    const char* serial = serial_number.c_str();
    auto status = DeviceStatus::FromKinesis(jog
        ? CC_GetJogVelParams(serial, &device_acceleration, &device_speed)
        : CC_GetVelParams(serial, &device_acceleration, &device_speed));
    if (!status.ok()) return status;
    // Compare device units, so equivalent real values do not cause redundant writes.
    if (speed.value_or(device_speed) == device_speed &&
        acceleration.value_or(device_acceleration) == device_acceleration)
        return DeviceStatus::Ok();
    return DeviceStatus::FromKinesis(jog
        ? CC_SetJogVelParams(serial, acceleration.value_or(device_acceleration), speed.value_or(device_speed))
        : CC_SetVelParams(serial, acceleration.value_or(device_acceleration), speed.value_or(device_speed)));
}

}

KDC101::PositionResult KDC101::GetPosition()
{
    int device_unit = CC_GetPosition(serial_number_.c_str());

    double position_real;
    DeviceStatus conversion_status = DeviceStatus::FromKinesis(CC_GetRealValueFromDeviceUnit(serial_number_.c_str(), device_unit, &position_real, kUnitTypeDistance));

    if (!conversion_status.ok()) {
        return std::unexpected(conversion_status);
    }

    return position_real;
}


DeviceStatus KDC101::SetMoveVelocity(VelocityParameters parameters) {
    return ApplyVelocity(serial_number_, parameters, false);
}

KDC101::PositionResult KDC101::GetDistanceResolution() {
    double resolution = 0;
    const auto status = DeviceStatus::FromKinesis(CC_GetRealValueFromDeviceUnit(
        serial_number_.c_str(), 1, &resolution, kUnitTypeDistance));
    if (!status.ok()) return std::unexpected(status);
    if (!std::isfinite(resolution) || resolution <= 0)
        return std::unexpected(DeviceStatus::FromKinesis(FT_InvalidParameter));
    return resolution;
}

DeviceStatus KDC101::SetJogVelocity(VelocityParameters parameters) {
    return ApplyVelocity(serial_number_, parameters, true);
}

DeviceStatus KDC101::SetHomingSpeed(double speed) {
    auto converted = ConvertMotionParameter(serial_number_, speed, kUnitTypeSpeed);
    if (!converted) return converted.error();
    return DeviceStatus::FromKinesis(CC_SetHomingVelocity(serial_number_.c_str(), static_cast<unsigned int>(*converted)));
}

DeviceStatus KDC101::SetJogStepSize(double step) {
    auto converted = ConvertMotionParameter(serial_number_, step, kUnitTypeDistance);
    if (!converted) return converted.error();
    return DeviceStatus::FromKinesis(CC_SetJogStepSize(serial_number_.c_str(), static_cast<unsigned int>(*converted)));
}

DeviceStatus KDC101::SetBacklash(double distance_mm) {
    if (!std::isfinite(distance_mm) || distance_mm < 0.0)
        return DeviceStatus::FromKinesis(FT_InvalidParameter).WithContext("SetBacklash: expected nonnegative millimetres");
    int units = 0;
    if (distance_mm > 0.0) {
        // Bound the conversion before asking the SDK to store it in a signed int.
        const auto resolution = GetDistanceResolution();
        if (!resolution) return resolution.error();
        if (distance_mm / *resolution > (std::numeric_limits<int>::max)())
            return DeviceStatus::FromKinesis(FT_InvalidParameter).WithContext("SetBacklash: distance out of range");
        const auto converted = ConvertMotionParameter(serial_number_, distance_mm, kUnitTypeDistance);
        if (!converted) return converted.error().WithContext("SetBacklash");
        units = *converted;
    }
    return DeviceStatus::FromKinesis(CC_SetBacklash(serial_number_.c_str(), units)).WithContext("CC_SetBacklash");
}

DeviceStatus KDC101::SaveSettingsSnapshot(const char* phase) {
    // Only called during connection: no user motion owns this message queue yet.
    CC_ClearMessageQueue(serial_number_.c_str());
    auto status = DeviceStatus::FromKinesis(CC_RequestSettings(serial_number_.c_str()));
    if (!status.ok()) return status.WithContext("Request controller settings snapshot");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    bool received = false;
    while (std::chrono::steady_clock::now() < deadline) {
        WORD type = 0, id = 0;
        DWORD data = 0;
        if (CC_GetNextMessage(serial_number_.c_str(), &type, &id, &data)) {
            // Kinesis GenericDevice: settingsInitialized / settingsUpdated.
            if (type == 0 && (id == 0 || id == 1)) { received = true; break; }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    if (!received) return DeviceStatus::Timeout(serial_number_, 0).WithContext("Waiting for controller settings snapshot");
    int acceleration = 0, speed = 0, jog_acceleration = 0, jog_speed = 0;
    status = DeviceStatus::FromKinesis(CC_GetVelParams(serial_number_.c_str(), &acceleration, &speed));
    if (!status.ok()) return status.WithContext("Snapshot move velocity");
    status = DeviceStatus::FromKinesis(CC_GetJogVelParams(serial_number_.c_str(), &jog_acceleration, &jog_speed));
    if (!status.ok()) return status.WithContext("Snapshot jog velocity");
    try {
        const auto path = std::filesystem::temp_directory_path() / "confo-quanta-settings.log";
        std::ofstream file(path, std::ios::app);
        const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        // Raw units preserve the old values without assuming the new stage calibration.
        file << timestamp << " serial=" << serial_number_ << " phase=" << phase
             << " units=device backlash=" << CC_GetBacklash(serial_number_.c_str())
             << " move_speed=" << speed << " move_acceleration=" << acceleration
             << " jog_speed=" << jog_speed << " jog_acceleration=" << jog_acceleration
             << " jog_step=" << CC_GetJogStepSize(serial_number_.c_str())
             << " homing_speed=" << CC_GetHomingVelocity(serial_number_.c_str()) << '\n';
        file.flush();
        if (!file) return DeviceStatus::FailedToLoadSettings(serial_number_).WithContext("Cannot write " + path.string());
    } catch (const std::exception& error) {
        return DeviceStatus::FailedToLoadSettings(serial_number_).WithContext(std::string("Settings snapshot: ") + error.what());
    }
    return DeviceStatus::Ok();
}

DeviceStatus KDC101::SetJogMode(JogMode mode, StopMode stop_mode) {
    if ((mode != JogMode::kSingleStep && mode != JogMode::kContinuous) ||
        (stop_mode != StopMode::kProfiled && stop_mode != StopMode::kImmediate))
        return DeviceStatus::FromKinesis(FT_InvalidParameter);
    return DeviceStatus::FromKinesis(CC_SetJogMode(serial_number_.c_str(),
        mode == JogMode::kSingleStep ? MOT_JogModes::MOT_SingleStep : MOT_JogModes::MOT_Continuous,
        stop_mode == StopMode::kProfiled ? MOT_StopModes::MOT_Profiled : MOT_StopModes::MOT_Immediate));
}

DeviceStatus KDC101::StartHome() {
    return DeviceStatus::FromKinesis(CC_Home(serial_number_.c_str()));
}

DeviceStatus KDC101::StartJog(Direction dir)
{
    if (dir != Direction::kForward && dir != Direction::kBackward)
        return DeviceStatus::FromKinesis(FT_InvalidParameter);
    MOT_TravelDirection mot_direction;

    if (dir == Direction::kForward) {
        mot_direction = MOT_TravelDirection::MOT_Forwards;
    } else {
        mot_direction = MOT_TravelDirection::MOT_Backwards;
    }

    return DeviceStatus::FromKinesis(CC_MoveJog(serial_number_.c_str(), mot_direction));
}


DeviceStatus KDC101::StartDrive(Direction dir)
{
    if (dir != Direction::kForward && dir != Direction::kBackward)
        return DeviceStatus::FromKinesis(FT_InvalidParameter);

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


DeviceStatus KDC101::StartMoveAbsolute(double position)
{
    if (!std::isfinite(position)) return DeviceStatus::FromKinesis(FT_InvalidParameter);

    // convert to device units
    int device_unit;
    DeviceStatus conversion_status = DeviceStatus::FromKinesis(CC_GetDeviceUnitFromRealValue(serial_number_.c_str(),
                                                                                             position,
                                                                                             &device_unit,
                                                                                             kUnitTypeDistance));

    if (!conversion_status.ok()) {
        return conversion_status;
    }

    return DeviceStatus::FromKinesis(CC_MoveToPosition(serial_number_.c_str(), device_unit));
}


DeviceStatus KDC101::StartMoveRelative(double distance)
{
    if (!std::isfinite(distance)) {
        return DeviceStatus::FromKinesis(FT_InvalidParameter);
    }

    int device_units = 0;
    auto conversion_status = DeviceStatus::FromKinesis(CC_GetDeviceUnitFromRealValue(serial_number_.c_str(), distance, &device_units, kUnitTypeDistance));
    if (!conversion_status.ok()) {
        return conversion_status;
    }

    // A zero displacement may never produce the completion event the worker awaits.
    if (device_units == 0) return DeviceStatus::MotionBelowResolution(serial_number_);
    const int position_before = CC_GetPosition(serial_number_.c_str());
    const short result = CC_MoveRelative(serial_number_.c_str(), device_units);
    TraceRelativeMove(serial_number_, distance, device_units, position_before, result);
    return DeviceStatus::FromKinesis(result);
}


std::expected<MotorStatus, DeviceStatus> KDC101::GetStatus()
{
    auto status = CheckConnection();
    if (!status.ok()) {
        return std::unexpected(status);
    }

    const auto bits = CC_GetStatusBits(serial_number_.c_str());
    MotorStatus status_flags;
    status_flags.forward_limit_switch = (bits & 0x00000001u) != 0;
    status_flags.reverse_limit_switch = (bits & 0x00000002u) != 0;
    status_flags.moving_forward = (bits & 0x00000010u) != 0;
    status_flags.moving_reverse = (bits & 0x00000020u) != 0;
    status_flags.jogging_forward = (bits & 0x00000040u) != 0;
    status_flags.jogging_reverse = (bits & 0x00000080u) != 0;
    status_flags.homing = (bits & 0x00000200u) != 0;
    status_flags.homed = (bits & 0x00000400u) != 0;
    status_flags.active = (bits & 0x20000000u) != 0;
    status_flags.channel_enabled = (bits & 0x80000000u) != 0;
    return status_flags;
}

DeviceStatus KDC101::CheckConnection() const
{
    if (!connected_ || !CC_CheckConnection(serial_number_.c_str())) {
        return DeviceStatus::NotConnected(serial_number_);
    }
    return DeviceStatus::Ok();
}


KDC101::EventResult KDC101::GetNextEvent()
{
    auto status = CheckConnection();
    if (!status.ok()) {
        return std::unexpected(status);
    }

    WORD type = 0;
    WORD id = 0;
    DWORD data = 0;
    if (!CC_GetNextMessage(serial_number_.c_str(), &type, &id, &data)) {
        status = CheckConnection();
        if (!status.ok()) {
            return std::unexpected(status);
        }
        return std::optional<MotorEvent>{};
    }

    constexpr WORD kGenericMotorMessageType = 2;
    constexpr WORD kHomedMessageId = 0;
    constexpr WORD kMovedMessageId = 1;
    constexpr WORD kStoppedMessageId = 2;

    if (type == kGenericMotorMessageType) {
        switch (id) {
            case kHomedMessageId: return MotorEvent::kHomed;
            case kMovedMessageId: return MotorEvent::kMoveCompleted;
            case kStoppedMessageId: return MotorEvent::kStopped;
        }
    }
    return MotorEvent::kOther;
}


DeviceStatus KDC101::ClearMessageQueue()
{
    auto status = CheckConnection();
    if (!status.ok()) {
        return status;
    }
    CC_ClearMessageQueue(serial_number_.c_str());
    return DeviceStatus::Ok();
}

} // namespace thorlabs
