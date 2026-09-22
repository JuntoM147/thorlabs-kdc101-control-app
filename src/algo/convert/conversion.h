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
  int PixelCount() const noexcept { return pixel_count_; }

 private:
  std::vector<std::vector<std::uint8_t>> pixels_;
  int pixel_count_ = 0;
};

// Pure white RGB pixels become 0; every other colour becomes 1
[[nodiscard]] std::expected<BinaryMatrix, std::string> Convert(const QImage& image);

}  // namespace algo

#endif  // ALGO_CONVERT_CONVERSION_H_
