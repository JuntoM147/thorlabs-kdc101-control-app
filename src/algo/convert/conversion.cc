#include "conversion.h"

#include <QColor>
#include <QImage>

namespace algo {

BinaryMatrix::BinaryMatrix(int width, int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  pixels_.resize(height);
  for (auto& row : pixels_) {
    row.resize(width, 0);
  }
}

int BinaryMatrix::Width() const noexcept {
  return pixels_.empty() ? 0 : static_cast<int>(pixels_.front().size());
}

int BinaryMatrix::Height() const noexcept {
  return static_cast<int>(pixels_.size());
}

std::uint8_t BinaryMatrix::At(int x, int y) const {
  return pixels_.at(y).at(x);
}

void BinaryMatrix::Set(int x, int y, bool expose) {
  pixels_.at(y).at(x) = static_cast<std::uint8_t>(expose);
}

std::expected<BinaryMatrix, std::string> Convert(const QImage& image) {
  if (image.isNull()) {
    return std::unexpected("Cannot convert a null image");
  }

  try {
    BinaryMatrix result(image.width(), image.height());
    for (int y = 0; y < image.height(); ++y) {
      for (int x = 0; x < image.width(); ++x) {
        const QColor pixel = image.pixelColor(x, y);
        const bool is_white = pixel.red() == 255 && pixel.green() == 255 && pixel.blue() == 255;
        result.Set(x, y, !is_white);
      }
    }
    return result;
  } catch (...) {
    // TODO: Remove exceptions from code base
    return std::unexpected("Image conversion failed");
  }
}

}  // namespace algo
