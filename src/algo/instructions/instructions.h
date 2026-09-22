#ifndef ALGO_INSTRUCTIONS_INSTRUCTIONS_H_
#define ALGO_INSTRUCTIONS_INSTRUCTIONS_H_

#include <variant>
#include <vector>

namespace algo {

struct PixelPosition {
  int x = 0;
  int y = 0;
};

struct MoveRelative {
  int dx = 0;
  int dy = 0;
};

enum class Action {
  kLaserOn,
  kLaserOff,
  kWait,
};

using Instruction = std::variant<Action, MoveRelative>;
using Program = std::vector<Instruction>;

}  // namespace algo

#endif  // ALGO_INSTRUCTIONS_INSTRUCTIONS_H_
