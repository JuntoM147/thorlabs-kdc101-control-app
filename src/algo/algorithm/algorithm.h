#ifndef ALGO_ALGORITHM_ALGORITHM_H_
#define ALGO_ALGORITHM_ALGORITHM_H_

#include "../convert/conversion.h"
#include "../instructions/instructions.h"

namespace algo {

enum class Direction { kPositiveX, kNegativeX, kPositiveY, kNegativeY };

[[nodiscard]] Program GenerateInstructions(
    PixelPosition start,
    BinaryMatrix matrix,
    double pixel_size_mm,
    Direction direction = Direction::kPositiveX);

}  // namespace algo

#endif  // ALGO_ALGORITHM_ALGORITHM_H_
