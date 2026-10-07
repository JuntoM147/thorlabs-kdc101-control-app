#include "scan_control.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>
#include <algorithm>
#include <climits>
#include <cmath>

namespace ui {

QWidget* CreateScanControlSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Scan control"), parent);
  section->setObjectName(QStringLiteral("scan_control"));

  auto* layout = new QVBoxLayout(section);
  layout->setSpacing(8);

  auto* settings = new QGridLayout();
  settings->setHorizontalSpacing(16);
  settings->setVerticalSpacing(6);
  settings->setColumnStretch(1, 1);
  settings->setColumnStretch(3, 1);
  auto* start_x = new QLineEdit(QStringLiteral("0"), section);
  auto* start_y = new QLineEdit(QStringLiteral("0"), section);
  start_x->setAccessibleName(QObject::tr("Starting pixel X"));
  start_y->setAccessibleName(QObject::tr("Starting pixel Y"));
  auto* x_label = new QLabel(QObject::tr("Start pixel X"), section);
  auto* y_label = new QLabel(QObject::tr("Start pixel Y"), section);
  x_label->setBuddy(start_x);
  y_label->setBuddy(start_y);
  settings->addWidget(x_label, 0, 0);
  settings->addWidget(start_x, 0, 1);
  settings->addWidget(y_label, 0, 2);
  settings->addWidget(start_y, 0, 3);
  for (auto* field : {start_x, start_y}) {
    field->setValidator(new QIntValidator(0, INT_MAX, field));
    field->setAlignment(Qt::AlignRight);
    field->setToolTip(
        QObject::tr("Zero-based image coordinate. Click Set start to apply."));
  }
  auto* set_start = new QPushButton(QObject::tr("Set start"), section);
  set_start->setAccessibleName(QObject::tr("Set start pixel"));
  set_start->setToolTip(
      QObject::tr("Map this image pixel to the current stage position."));
  settings->addWidget(set_start, 0, 4);
  layout->addLayout(settings);
  auto* physical_size = new QLabel(section);
  physical_size->setAccessibleName(QObject::tr("Physical scan size"));
  physical_size->setToolTip(QObject::tr(
      "Image width and height multiplied by pixel size. Motor targets refer "
      "to pixel centres."));
  const auto update_physical_size = [&view, physical_size] {
    const auto size = view.ScanSizeMillimetres();
    if (!std::isfinite(size.width()) || !std::isfinite(size.height()) ||
        size.width() <= 0 || size.height() <= 0) {
      physical_size->setText(QObject::tr("Physical size: set an image and valid pixel size."));
    } else {
      physical_size->setText(QObject::tr("Physical size: %1 × %2 mm (%3 µm/pixel)")
          .arg(size.width(), 0, 'g', 8).arg(size.height(), 0, 'g', 8)
          .arg(view.PixelSizeMicrometres(), 0, 'g', 8));
    }
  };
  QObject::connect(&view, &MainWindow::ScanAvailabilityChanged, section,
                   update_physical_size);
  update_physical_size();
  layout->addWidget(physical_size);
  auto* preview_buttons = new QHBoxLayout();
  auto* direction = new QComboBox(section);
  direction->setAccessibleName(QObject::tr("Scan direction"));
  direction->addItem(QObject::tr("Choose direction…"));
  direction->addItem(QObject::tr("Left to right"),
                     static_cast<int>(algo::Direction::kPositiveX));
  direction->addItem(QObject::tr("Right to left"),
                     static_cast<int>(algo::Direction::kNegativeX));
  direction->addItem(QObject::tr("Up to down"),
                     static_cast<int>(algo::Direction::kPositiveY));
  direction->addItem(QObject::tr("Down to up"),
                     static_cast<int>(algo::Direction::kNegativeY));
  preview_buttons->addWidget(direction, 1);
  QObject::connect(direction, &QComboBox::currentIndexChanged, section,
                   [&view, direction](int index) {
                     if (index == 0)
                       view.SetScanDirection(std::nullopt);
                     else
                       view.SetScanDirection(static_cast<algo::Direction>(
                           direction->currentData().toInt()));
                   });
  QObject::connect(&view, &MainWindow::ScanInputsEnabled, direction,
                   &QWidget::setEnabled);
  auto* preview_scan = new QPushButton(QObject::tr("Preview scan"), section);
  auto* stop_preview = new QPushButton(QObject::tr("Stop preview"), section);
  auto* clear_preview = new QPushButton(QObject::tr("Clear preview"), section);
  for (auto* button : {preview_scan, stop_preview, clear_preview}) {
    button->setAccessibleName(button->text());
    preview_buttons->addWidget(button, 1);
  }
  layout->addLayout(preview_buttons);
  QObject::connect(preview_scan, &QPushButton::clicked, &view,
                   &MainWindow::PreviewScanRequested);
  QObject::connect(stop_preview, &QPushButton::clicked, &view,
                   &MainWindow::StopPreviewRequested);
  QObject::connect(clear_preview, &QPushButton::clicked, &view,
                   &MainWindow::ClearPreviewRequested);
  const auto update_preview_controls = [&view, preview_scan, stop_preview,
                                        clear_preview] {
    const bool enabled = view.CanPreviewScan();
    preview_scan->setEnabled(enabled);
    stop_preview->setEnabled(enabled);
    clear_preview->setEnabled(enabled);
  };
  QObject::connect(&view, &MainWindow::ScanAvailabilityChanged, section,
                   update_preview_controls);
  update_preview_controls();
  auto update_start_controls = [&view, start_x, start_y, set_start] {
    const bool enabled = view.CanSetStartPixel();
    start_x->setEnabled(enabled);
    start_y->setEnabled(enabled);
    set_start->setEnabled(enabled);
  };
  QObject::connect(&view, &MainWindow::ScanAvailabilityChanged, section,
                   update_start_controls);
  update_start_controls();
  QObject::connect(
      set_start, &QPushButton::clicked, section, [&view, start_x, start_y] {
        bool x_ok = false;
        bool y_ok = false;
        const int x = start_x->text().toInt(&x_ok);
        const int y = start_y->text().toInt(&y_ok);
        if (!x_ok || !y_ok || !start_x->hasAcceptableInput() ||
            !start_y->hasAcceptableInput()) {
          view.ShowMessage(
              QObject::tr(
                  "Set start: Enter nonnegative whole numbers for X and Y."),
              true);
          return;
        }
        view.SetStartPixel(x, y);
      });

  auto* progress = new QProgressBar(section);
  progress->setRange(0, 100);
  progress->setValue(0);
  progress->setMinimumHeight(26);
  progress->setAccessibleName(QObject::tr("Scan progress"));
  layout->addWidget(progress);

  auto* remaining_time = new QLabel(section);
  remaining_time->setAccessibleName(
      QObject::tr("Estimated scan time remaining"));
  remaining_time->setAlignment(Qt::AlignCenter);
  layout->addWidget(remaining_time);

  auto* estimated_time = new QLabel(section);
  estimated_time->setAccessibleName(QObject::tr("Scan state"));
  estimated_time->setAlignment(Qt::AlignCenter);
  estimated_time->setToolTip(QObject::tr("Reported scan phase."));
  layout->addWidget(estimated_time);

  auto* buttons = new QHBoxLayout();
  buttons->setSpacing(12);

  auto* start = new QPushButton(QObject::tr("Start"), section);
  start->setIcon(section->style()->standardIcon(QStyle::SP_MediaPlay));
  start->setProperty("primary", true);
  start->setAccessibleName(QObject::tr("Start scan"));
  start->setToolTip(QObject::tr(
      "The applied start pixel maps to the current stage position."));
  start->setEnabled(false);
  start->setMinimumSize(88, 30);
  start->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  buttons->addWidget(start, 1);

  auto* stop = new QPushButton(QObject::tr("Stop"), section);
  stop->setIcon(section->style()->standardIcon(QStyle::SP_MediaStop));
  stop->setAccessibleName(QObject::tr("Stop scan"));
  stop->setToolTip(QObject::tr("No scan is running."));
  stop->setEnabled(false);
  stop->setMinimumSize(88, 30);
  stop->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  buttons->addWidget(stop, 1);

  auto* pause = new QPushButton(QObject::tr("Pause"), section);
  pause->setAccessibleName(QObject::tr("Pause scan"));
  pause->setEnabled(false);
  auto* resume = new QPushButton(QObject::tr("Resume"), section);
  resume->setAccessibleName(QObject::tr("Resume scan"));
  resume->setEnabled(false);
  buttons->insertWidget(1, pause, 1);
  buttons->insertWidget(2, resume, 1);
  layout->addLayout(buttons);

  auto* reset = new QPushButton(QObject::tr("Reset"), section);
  reset->setAccessibleName(QObject::tr("Reset scan"));
  reset->setToolTip(
      QObject::tr("Turn laser OFF, stop all motors and return to manual "
                  "control. Keeps the image; clears progress and the applied "
                  "starting pixel. Does not home or move to zero."));
  reset->setEnabled(false);
  buttons->addWidget(reset, 1);
  QObject::connect(reset, &QPushButton::clicked, &view,
                   &MainWindow::RequestResetScan);

  QObject::connect(start, &QPushButton::clicked, &view,
                   &MainWindow::RequestScan);
  QObject::connect(pause, &QPushButton::clicked, &view,
                   &MainWindow::PauseScanRequested);
  QObject::connect(resume, &QPushButton::clicked, &view,
                   &MainWindow::ResumeScanRequested);
  QObject::connect(stop, &QPushButton::clicked, &view,
                   &MainWindow::CancelScanRequested);
  QObject::connect(
      &view, &MainWindow::ScanAvailabilityChanged, section, [=, &view] {
        using ui::ScanPhase;
        const auto phase = view.ScanPhase();
        reset->setEnabled(view.CanResetScan());
        const auto blockers = view.ScanStartBlockers();
        start->setEnabled(blockers.isEmpty());
        start->setToolTip(blockers.isEmpty()
                              ? QObject::tr("The applied start pixel maps to "
                                            "the current stage position.")
                              : blockers.join(QStringLiteral("\n")));
        pause->setEnabled(view.HasWorkers() && phase == ScanPhase::kRunning);
        resume->setEnabled(view.HasWorkers() && phase == ScanPhase::kPaused);
        stop->setEnabled(view.HasWorkers() && phase != ScanPhase::kIdle &&
                         phase != ScanPhase::kFailed &&
                         phase != ScanPhase::kStopping);
        stop->setToolTip(
            QObject::tr("Cancel the scan and stop its device operations."));
      });
  QObject::connect(
      &view, &MainWindow::ScanDisplayChanged, section,
      [=](ui::ScanState state) {
        using ui::ScanPhase;
        const int percent =
            state.total_instructions == 0
                ? 0
                : static_cast<int>(100.0L *
                                   std::min(state.completed_instructions,
                                            state.total_instructions) /
                                   state.total_instructions);
        progress->setValue(percent);
        const bool show_estimate = state.phase == ScanPhase::kRunning ||
                                   state.phase == ScanPhase::kPausing ||
                                   state.phase == ScanPhase::kPaused;
        if (!show_estimate) {
          remaining_time->clear();
        } else if (!state.estimated_remaining) {
          remaining_time->setText(QObject::tr("Estimating time remaining…"));
        } else {
          const auto seconds = state.estimated_remaining->count();
          remaining_time->setText(QObject::tr("About %1 min %2 sec remaining")
                                      .arg(seconds / 60)
                                      .arg(seconds % 60));
        }
        QString phase;
        switch (state.phase) {
          case ScanPhase::kIdle:
            break;
          case ScanPhase::kRunning:
            phase = QObject::tr("Scanning");
            break;
          case ScanPhase::kPausing:
            phase = QObject::tr("Pausing");
            break;
          case ScanPhase::kPaused:
            phase = QObject::tr("Paused");
            break;
          case ScanPhase::kStopping:
            phase = QObject::tr("Stopping");
            break;
          case ScanPhase::kFailed:
            phase = QObject::tr("Failed");
            break;
        }
        estimated_time->setText(phase);
      });

  return section;
}

}  // namespace ui
