#ifndef UI_SCAN_PREVIEW_H_
#define UI_SCAN_PREVIEW_H_

#include <QLabel>
#include <QPainterPath>
#include <QTimer>
#include <optional>

#include "algo/instructions/instructions.h"

namespace ui {
// A visual simulation only; this widget has no connection to hardware.
class ScanPreview : public QLabel {
 public:
  explicit ScanPreview(QWidget* parent);
  void SetStart(std::optional<QPoint> start);
  void Start(algo::Program program);
  void Stop();
  void Clear();
  QSize sizeHint() const override { return {320, 240}; }
  QSize minimumSizeHint() const override { return {180, 180}; }

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  void Advance();
  QTimer timer_;
  std::optional<QPoint> start_;
  algo::Program program_;
  std::size_t instruction_ = 0;
  QPointF cursor_;
  std::optional<QPointF> target_;
  bool laser_on_ = false;
  QPainterPath drawing_;
};
}  // namespace ui
#endif
