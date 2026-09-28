#ifndef UI_ROUTE_PREVIEW_ROUTE_PREVIEW_H_
#define UI_ROUTE_PREVIEW_ROUTE_PREVIEW_H_

#include <vector>

#include <QPainterPath>
#include <QPointF>

#include "../../../algo/instructions/instructions.h"

class QPainter;
class QTransform;

namespace ui {

// Cached overlay geometry in image pixel coordinates, independent of widgets.
class RoutePreview {
 public:
  // An empty program clears the overlay. Pixel coordinates refer to pixel centers.
  void SetRoute(const algo::Program& program, algo::PixelPosition start);
  // The caller supplies image-to-display mapping and clipping. Painter state is preserved.
  void Draw(QPainter& painter, const QTransform& image_to_display) const;

 private:
  QPainterPath exposure_;
  QPainterPath travel_;
  std::vector<QPointF> dots_;
  QPointF start_;
  bool visible_ = false;
};

}  // namespace ui

#endif  // UI_ROUTE_PREVIEW_ROUTE_PREVIEW_H_
