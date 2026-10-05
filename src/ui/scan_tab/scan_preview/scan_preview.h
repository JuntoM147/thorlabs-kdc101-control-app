#ifndef UI_SCAN_PREVIEW_H_
#define UI_SCAN_PREVIEW_H_

#include <QLabel>
#include <QPainterPath>
#include <QTimer>
#include <optional>

#include "algo/instructions/instructions.h"

namespace ui {
// Animates instructions or shows scan progress; never commands hardware.
class ScanPreview : public QLabel {
 public:
  explicit ScanPreview(QWidget* parent);
  void SetStart(std::optional<QPoint> start);
  void Start(algo::Program program);
  void Stop();
  void Clear();
  void FollowInstructions(algo::Program program);
  void ShowThrough(std::size_t instruction_count);
  void EndFollowing();
  QSize sizeHint() const override { return {320, 240}; }
  QSize minimumSizeHint() const override { return {180, 180}; }

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  void Advance();
  void ApplyAction(algo::Action action);
  QTimer timer_;
  std::optional<QPoint> start_;
  algo::Program program_;
  std::size_t instruction_ = 0;
  QPointF cursor_;
  std::optional<QPointF> target_;
  bool laser_on_ = false;
  bool following_instructions_ = false;
  bool cursor_visible_ = false;
  QPainterPath drawing_;
};
}  // namespace ui
#endif
