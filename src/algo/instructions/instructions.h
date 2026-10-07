#ifndef ALGO_INSTRUCTIONS_INSTRUCTIONS_H_
#define ALGO_INSTRUCTIONS_INSTRUCTIONS_H_

#include <variant>
#include <vector>

namespace algo {

struct PixelPosition {
  int x = 0;
  int y = 0;
};

struct MoveAbsolute {
  // Image coordinates used by the preview.
  int x = 0;
  int y = 0;
  // Physical targets relative to the stage position mapped to the start pixel.
  double x_mm = 0;
  double y_mm = 0;
};

enum class Action {
  kLaserOn,
  kLaserOff,
  kWait,
};

using Instruction = std::variant<Action, MoveAbsolute>;
using Program = std::vector<Instruction>;

}  // namespace algo

#endif  // ALGO_INSTRUCTIONS_INSTRUCTIONS_H_
