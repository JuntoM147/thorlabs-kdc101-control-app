#ifndef ALGO_ALGORITHM_ALGORITHM_H_
#define ALGO_ALGORITHM_ALGORITHM_H_

#include "../convert/conversion.h"
#include "../instructions/instructions.h"

namespace algo {

[[nodiscard]] Program GenerateInstructions(PixelPosition start, BinaryMatrix matrix);

}  // namespace algo

#endif  // ALGO_ALGORITHM_ALGORITHM_H_
