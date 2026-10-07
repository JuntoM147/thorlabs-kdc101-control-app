#ifndef KDC101_DEFAULTS_H_
#define KDC101_DEFAULTS_H_

namespace thorlabs {

// KDC101 Z825 defaults to load in at device startup
inline constexpr char kDefaultStageSettings[] = "Z825";
inline constexpr int kDefaultPollingIntervalMs = 200;
inline constexpr double kDefaultMoveSpeedMmPerSecond = 0.1;  // 100 um/s
// Nominal 0.1 s ramp to the default speed; short moves may not reach it.
inline constexpr double kDefaultMoveAccelerationMmPerSecondSquared = 1.0;
inline constexpr double kDefaultJogSpeedMmPerSecond = 0.1;  // 100 um/s
inline constexpr double kDefaultJogAccelerationMmPerSecondSquared = 1.0;
inline constexpr double kDefaultJogStepMm = 0.1;
inline constexpr double kDefaultHomingSpeedMmPerSecond = 0.1;
// Experimental Z825 compensation overshoot: 100 um, independent of pixel size.
// Reduced from the documented Z825B 300 um compensation distance.
// https://www.thorlabs.com/catalogpages/Obsolete/2023/Z825B.pdf
// Negative moves can overshoot before returning; expose in the positive direction.
inline constexpr double kDefaultBacklashMm = 0.1;

}  // namespace thorlabs

#endif  // KDC101_DEFAULTS_H_
