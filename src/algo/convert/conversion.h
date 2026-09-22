#ifndef ALGO_CONVERT_CONVERSION_H_
#define ALGO_CONVERT_CONVERSION_H_

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

class QImage;

namespace algo {

// Rectangular matrix of 0s and 1s
class BinaryMatrix {
 public:
  BinaryMatrix() = default;
  BinaryMatrix(int width, int height);

  [[nodiscard]] int Width() const noexcept;
  [[nodiscard]] int Height() const noexcept;

  [[nodiscard]] std::uint8_t At(int x, int y) const;

  void Set(int x, int y, bool expose);

 private:
  std::vector<std::vector<std::uint8_t>> pixels_;
};

// Pure white RGB pixels become 0; every other colour becomes 1.
// Assumes opaque black-and-white input; alpha is ignored. Preserves dimensions.
[[nodiscard]] std::expected<BinaryMatrix, std::string> Convert(const QImage& image);

}  // namespace algo

#endif  // ALGO_CONVERT_CONVERSION_H_
