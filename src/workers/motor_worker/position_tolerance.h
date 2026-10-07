#ifndef WORKERS_MOTOR_WORKER_POSITION_TOLERANCE_H_
#define WORKERS_MOTOR_WORKER_POSITION_TOLERANCE_H_

#include <cmath>
#include <limits>

namespace workers {
inline constexpr double kPositionToleranceMm = 0.05 / 1000.0;

inline bool WithinPositionTolerance(double position_mm, double target_mm) {
  if (!std::isfinite(position_mm) || !std::isfinite(target_mm)) return false;
  // Permit only floating-point subtraction noise at the inclusive boundary.
  const double roundoff = std::numeric_limits<double>::epsilon() *
      (std::abs(position_mm) + std::abs(target_mm));
  return std::abs(position_mm - target_mm) <= kPositionToleranceMm + roundoff;
}
}  // namespace workers
#endif
