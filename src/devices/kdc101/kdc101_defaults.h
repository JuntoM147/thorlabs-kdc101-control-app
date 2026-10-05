#ifndef KDC101_DEFAULTS_H_
#define KDC101_DEFAULTS_H_

namespace thorlabs {

// KDC101 Z825 defaults to load in at device startup
inline constexpr char kDefaultStageSettings[] = "Z825";
inline constexpr int kDefaultPollingIntervalMs = 200;
inline constexpr double kDefaultMoveSpeedMmPerSecond = 0.01; // 10 um/s
inline constexpr double kDefaultMoveAccelerationMmPerSecondSquared = 1.5;
inline constexpr double kDefaultJogSpeedMmPerSecond = 0.01; // 10 um/s
inline constexpr double kDefaultJogAccelerationMmPerSecondSquared = 2.0;
inline constexpr double kDefaultJogStepMm = 0.1;
inline constexpr double kDefaultHomingSpeedMmPerSecond = 1.0;
inline constexpr double kDefaultBacklashMm = 0.0;

}  // namespace thorlabs

#endif  // KDC101_DEFAULTS_H_
