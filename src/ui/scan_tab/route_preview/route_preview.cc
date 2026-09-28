#include "scan_tab/route_preview/route_preview.h"

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QTransform>

namespace ui {

void RoutePreview::SetRoute(const algo::Program& program, algo::PixelPosition start) {
  exposure_ = QPainterPath();
  travel_ = QPainterPath();
  dots_.clear();
  visible_ = !program.empty();
  start_ = QPointF(start.x + 0.5, start.y + 0.5);
  QPointF position = start_;
  bool laser_on = false;
  for (const auto& instruction : program) {
    if (const auto* move = std::get_if<algo::MoveRelative>(&instruction)) {
      const QPointF next = position + QPointF(move->dx, move->dy);
      auto& path = laser_on ? exposure_ : travel_;
      path.moveTo(position);
      path.lineTo(next);
      position = next;
    } else {
      switch (std::get<algo::Action>(instruction)) {
        case algo::Action::kLaserOn: laser_on = true; break;
        case algo::Action::kLaserOff: laser_on = false; break;
        case algo::Action::kWait:
          if (laser_on) dots_.push_back(position);
          break;
      }
    }
  }
}

void RoutePreview::Draw(QPainter& painter, const QTransform& image_to_display) const {
  if (!visible_) return;
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(Qt::NoBrush);
  // Map endpoints directly: a two-axis instruction remains a straight segment.
  // Map geometry rather than the painter so widths and markers stay display-sized.
  painter.setPen(QPen(QColor("#007bce"), 2, Qt::DashLine));
  painter.drawPath(image_to_display.map(travel_));
  painter.setPen(QPen(QColor("#ed7400"), 2.5, Qt::SolidLine, Qt::RoundCap));
  painter.drawPath(image_to_display.map(exposure_));
  painter.setBrush(QColor("#ed7400"));
  painter.setPen(QPen(Qt::white, 1));
  for (const auto& dot : dots_) painter.drawEllipse(image_to_display.map(dot), 3.5, 3.5);
  painter.setBrush(Qt::NoBrush);
  painter.setPen(QPen(Qt::white, 5));
  painter.drawEllipse(image_to_display.map(start_), 5, 5);
  painter.setPen(QPen(QColor("#13843b"), 2.5));
  painter.drawEllipse(image_to_display.map(start_), 5, 5);
  painter.restore();
}

}  // namespace ui
