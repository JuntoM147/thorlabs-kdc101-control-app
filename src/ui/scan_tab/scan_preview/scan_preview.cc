#include "scan_preview.h"

#include <QPainter>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <utility>

namespace ui {
ScanPreview::ScanPreview(QWidget* parent) : QLabel(parent) {
  setText(tr("No image imported"));
  setAlignment(Qt::AlignCenter);
  setMinimumSize(180, 180);
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
  timer_.setInterval(20);
  connect(&timer_, &QTimer::timeout, this, &ScanPreview::Advance);
}

void ScanPreview::Stop() { timer_.stop(); }
void ScanPreview::Clear() { SetStart(start_); }

void ScanPreview::SetStart(std::optional<QPoint> start) {
  Stop();
  start_ = start;
  program_.clear();
  target_.reset();
  drawing_ = {};
  cursor_ = start.value_or(QPoint{});
  following_instructions_ = false;
  cursor_visible_ = false;
  update();
}

void ScanPreview::Start(algo::Program program) {
  if (!start_ || pixmap().isNull()) return;
  SetStart(start_);
  program_ = std::move(program);
  instruction_ = 0;
  laser_on_ = false;
  cursor_visible_ = true;
  timer_.start();
}

void ScanPreview::FollowInstructions(algo::Program program) {
  Start(std::move(program));
  Stop();
  following_instructions_ = true;
}

void ScanPreview::ShowThrough(std::size_t instruction_count) {
  if (!following_instructions_) return;
  const auto count = std::min(instruction_count, program_.size());
  while (instruction_ < count) {
    const auto& instruction = program_[instruction_++];
    if (const auto* move = std::get_if<algo::MoveRelative>(&instruction)) {
      cursor_ += QPointF(move->dx, move->dy);
      if (laser_on_) drawing_.lineTo(cursor_);
    } else {
      ApplyAction(std::get<algo::Action>(instruction));
    }
  }
  update();
}

void ScanPreview::EndFollowing() { following_instructions_ = false; }

void ScanPreview::ApplyAction(algo::Action action) {
  switch (action) {
    case algo::Action::kLaserOn:
      laser_on_ = true;
      drawing_.moveTo(cursor_);
      break;
    case algo::Action::kLaserOff:
      laser_on_ = false;
      break;
    case algo::Action::kWait:
      if (laser_on_) drawing_.addEllipse(cursor_, 0.4, 0.4);
      break;
  }
}
void ScanPreview::Advance() {
  if (instruction_ == program_.size()) {
    Stop();
    return;
  }
  const auto& instruction = program_[instruction_];
  if (const auto* move = std::get_if<algo::MoveRelative>(&instruction)) {
    if (!target_) target_ = cursor_ + QPointF(move->dx, move->dy);
    const QPointF delta = *target_ - cursor_;
    const double distance = std::hypot(delta.x(), delta.y());
    // Cross the longest image dimension in roughly two and a half seconds.
    const double step =
        std::max(1.0, std::max(pixmap().width(), pixmap().height()) / 120.0);
    cursor_ = distance <= step ? *target_ : cursor_ + delta * (step / distance);
    if (laser_on_) drawing_.lineTo(cursor_);
    if (distance <= step) {
      target_.reset();
      ++instruction_;
    }
  } else {
    ApplyAction(std::get<algo::Action>(instruction));
    ++instruction_;
  }
  update();
}

void ScanPreview::paintEvent(QPaintEvent* event) {
  const auto image = pixmap();
  if (image.isNull()) {
    QLabel::paintEvent(event);
    return;
  }
  const QRect available = contentsRect().adjusted(12, 12, -12, -12);
  if (available.isEmpty()) return;
  const QSize fitted =
      image.size().scaled(available.size(), Qt::KeepAspectRatio);
  const QRect target(
      available.center() - QPoint(fitted.width() / 2, fitted.height() / 2),
      fitted);
  QPainter painter(this);
  painter.setRenderHint(QPainter::SmoothPixmapTransform);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.drawPixmap(target, image);
  QTransform transform;
  transform.translate(target.x(), target.y());
  transform.scale(double(target.width()) / image.width(),
                  double(target.height()) / image.height());
  transform.translate(0.5, 0.5);  // Pixel coordinates refer to pixel centres.
  painter.setPen(QPen(QColor("#00a34a"), 2));
  painter.drawPath(transform.map(drawing_));
  if (start_) {
    painter.setPen(Qt::white);
    painter.setBrush(QColor("#126bf0"));
    painter.drawEllipse(transform.map(QPointF(*start_)), 5, 5);
    if (cursor_visible_) {
      painter.setBrush(QColor("#ff7b13"));
      painter.drawEllipse(transform.map(cursor_), 4, 4);
    }
  }
}
}  // namespace ui
