#include "algorithm.h"

#include <vector>
#include <cmath>
#include <stdexcept>

namespace algo {
namespace {

struct Stroke {
  PixelPosition start;
  PixelPosition end;
};

void RowMajorOrder(const BinaryMatrix& matrix,
                   int row,
                   std::vector<Stroke>& strokes) {
  int x = 0;
  while (x < matrix.Width()) {
    if (!matrix.At(x, row)) {
      ++x;
      continue;
    }

    const PixelPosition start{x, row};
    while (x < matrix.Width() && matrix.At(x, row)) ++x;
    strokes.push_back({start, {x - 1, row}});
  }
}

void RowMajorOrderReverse(const BinaryMatrix& matrix, int row,
                          std::vector<Stroke>& strokes) {
  int x = matrix.Width() - 1;
  while (x >= 0) {
    if (!matrix.At(x, row)) {
      --x;
      continue;
    }

    const PixelPosition start{x, row};
    while (x >= 0 && matrix.At(x, row)) --x;
    strokes.push_back({start, {x + 1, row}});
  }
}

void ColumnMajorOrder(const BinaryMatrix& matrix, int col,
                      std::vector<Stroke>& strokes) {
  int y = 0;
  while (y < matrix.Height()) {
    if (!matrix.At(col, y)) {
      ++y;
      continue;
    }

    const PixelPosition start{col, y};
    while (y < matrix.Height() && matrix.At(col, y)) ++y;
    strokes.push_back({start, {col, y - 1}});
  }
}

void ColumnMajorOrderReverse(const BinaryMatrix& matrix, int col,
                             std::vector<Stroke>& strokes) {
  int y = matrix.Height() - 1;
  while (y >= 0) {
    if (!matrix.At(col, y)) {
      --y;
      continue;
    }

    const PixelPosition start{col, y};
    while (y >= 0 && matrix.At(col, y)) --y;
    strokes.push_back({start, {col, y + 1}});
  }
}

std::vector<Stroke> FindStokes(const BinaryMatrix& matrix,
                               Direction direction) {
  std::vector<Stroke> strokes;

  // Empty matrix, no strokes to find
  if (matrix.Width() == 0 || matrix.Height() == 0) return strokes;

  switch (direction) {
    case Direction::kPositiveX:
      for (int row = 0; row < matrix.Height(); ++row) {
        RowMajorOrder(matrix, row, strokes);
      }
      break;
    case Direction::kNegativeX:
      for (int row = 0; row < matrix.Height(); ++row) {
        RowMajorOrderReverse(matrix, row, strokes);
      }
      break;
    case Direction::kPositiveY:
      for (int col = 0; col < matrix.Width(); ++col) {
        ColumnMajorOrder(matrix, col, strokes);
      }
      break;
    case Direction::kNegativeY:
      for (int col = 0; col < matrix.Width(); ++col) {
        ColumnMajorOrderReverse(matrix, col, strokes);
      }
      break;
  }

  return strokes;
}

}  // namespace

Program GenerateInstructions(PixelPosition start,
                             BinaryMatrix matrix,
                             double pixel_size_mm,
                             Direction direction) {
  if (!std::isfinite(pixel_size_mm) || pixel_size_mm <= 0)
    throw std::invalid_argument("Pixel size must be positive and finite.");
  const auto strokes = FindStokes(matrix, direction);

  Program program{Action::kLaserOff};
  const auto move_to = [start, pixel_size_mm](PixelPosition target) {
    return MoveAbsolute{target.x, target.y,
                        (double(target.x) - start.x) * pixel_size_mm,
                        (double(target.y) - start.y) * pixel_size_mm};
  };
  for (const auto& stroke : strokes) {
    program.push_back(move_to(stroke.start));

    program.push_back(Action::kLaserOn);

    if (stroke.start.x == stroke.end.x && stroke.start.y == stroke.end.y) {
      // Single pixel case
      program.push_back(Action::kWait);
    } else {
      program.push_back(move_to(stroke.end));
    }
    program.push_back(Action::kLaserOff);
  }
  return program;
}

}  // namespace algo
