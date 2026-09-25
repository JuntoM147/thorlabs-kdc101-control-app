#include "scan_control.h"
#include <algorithm>

#include <climits>
#include <QIntValidator>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>

namespace ui {

QWidget* CreateScanControlSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Scan control"), parent);
  section->setObjectName(QStringLiteral("scan_control"));

  auto* layout = new QVBoxLayout(section);
  layout->setSpacing(14);

  auto* settings = new QGridLayout();
  settings->setHorizontalSpacing(16);
  settings->setVerticalSpacing(6);
  settings->setColumnStretch(0, 1);
  settings->setColumnStretch(1, 1);
  auto* start_x = new QLineEdit(QStringLiteral("0"), section);
  auto* start_y = new QLineEdit(QStringLiteral("0"), section);
  start_x->setAccessibleName(QObject::tr("Starting pixel X"));
  start_y->setAccessibleName(QObject::tr("Starting pixel Y"));
  auto* x_label = new QLabel(QObject::tr("Starting pixel X"), section);
  auto* y_label = new QLabel(QObject::tr("Starting pixel Y"), section);
  x_label->setBuddy(start_x);
  y_label->setBuddy(start_y);
  settings->addWidget(x_label, 0, 0);
  settings->addWidget(y_label, 0, 1);
  settings->addWidget(start_x, 1, 0);
  settings->addWidget(start_y, 1, 1);
  for (auto* field : {start_x, start_y}) {
    field->setValidator(new QIntValidator(0, INT_MAX, field));
    field->setAlignment(Qt::AlignRight);
    field->setToolTip(QObject::tr("Zero-based image coordinate. Click Set start to apply."));

  }
  auto* set_start = new QPushButton(QObject::tr("Set start"), section);
  set_start->setAccessibleName(QObject::tr("Set start pixel"));
  set_start->setToolTip(QObject::tr("Map this image pixel to the current stage position."));
  settings->addWidget(set_start, 1, 2);
  layout->addLayout(settings);
  auto update_start_controls = [&view, start_x, start_y, set_start] {
    const bool enabled = view.CanSetStartPixel();
    start_x->setEnabled(enabled);
    start_y->setEnabled(enabled);
    set_start->setEnabled(enabled);
  };
  QObject::connect(&view, &MainWindow::ScanAvailabilityChanged, section, update_start_controls);
  update_start_controls();
  QObject::connect(set_start, &QPushButton::clicked, section, [&view, start_x, start_y] {
    bool x_ok = false;
    bool y_ok = false;
    const int x = start_x->text().toInt(&x_ok);
    const int y = start_y->text().toInt(&y_ok);
    if (!x_ok || !y_ok || !start_x->hasAcceptableInput() || !start_y->hasAcceptableInput()) {
      view.ShowMessage(QObject::tr("Set start: Enter nonnegative whole numbers for X and Y."), true);
      return;
    }
    view.SetStartPixel(x, y);
  });

  auto* progress = new QProgressBar(section);
  progress->setRange(0, 100);
  progress->setValue(0);
  progress->setMinimumHeight(36);
  progress->setAccessibleName(QObject::tr("Scan progress"));
  layout->addWidget(progress);

  auto* estimated_time = new QLabel(QObject::tr("Idle"), section);
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
  start->setToolTip(QObject::tr("The applied start pixel maps to the current stage position."));
  start->setEnabled(false);
  start->setMinimumSize(88, 40);
  start->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  buttons->addWidget(start, 1);

  auto* stop = new QPushButton(QObject::tr("Stop"), section);
  stop->setIcon(section->style()->standardIcon(QStyle::SP_MediaStop));
  stop->setAccessibleName(QObject::tr("Stop scan"));
  stop->setToolTip(QObject::tr("No scan is running."));
  stop->setEnabled(false);
  stop->setMinimumSize(88, 40);
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

  QObject::connect(start, &QPushButton::clicked, &view, &MainWindow::RequestScan);
  QObject::connect(pause, &QPushButton::clicked, &view, &MainWindow::PauseScanRequested);
  QObject::connect(resume, &QPushButton::clicked, &view, &MainWindow::ResumeScanRequested);
  QObject::connect(stop, &QPushButton::clicked, &view, &MainWindow::CancelScanRequested);
  QObject::connect(&view, &MainWindow::ScanAvailabilityChanged, section, [=, &view] {
    using application::ScanPhase;
    const auto phase = view.ScanPhase();
    const auto blockers = view.ScanStartBlockers();
    start->setEnabled(blockers.isEmpty());
    start->setToolTip(blockers.isEmpty()
        ? QObject::tr("The applied start pixel maps to the current stage position.")
        : blockers.join(QStringLiteral("\n")));
    pause->setEnabled(view.HasBackend() && phase == ScanPhase::kRunning);
    resume->setEnabled(view.HasBackend() && phase == ScanPhase::kPaused);
    stop->setEnabled(view.HasBackend() && phase != ScanPhase::kIdle &&
                     phase != ScanPhase::kFailed && phase != ScanPhase::kStopping);
    stop->setToolTip(QObject::tr("Cancel the scan and stop its device operations."));
  });
  QObject::connect(&view, &MainWindow::ScanDisplayChanged, section, [=](application::ScanState state) {
    using application::ScanPhase;
    const int percent = state.total_instructions == 0 ? 0
        : static_cast<int>(100.0L * std::min(state.completed_instructions, state.total_instructions)
                           / state.total_instructions);
    progress->setValue(percent);
    QString phase;
    switch (state.phase) {
      case ScanPhase::kIdle: phase = QObject::tr("Idle"); break;
      case ScanPhase::kWaiting: phase = QObject::tr("Waiting for manual operations"); break;
      case ScanPhase::kPreparing: phase = QObject::tr("Preparing"); break;
      case ScanPhase::kRunning: phase = QObject::tr("Scanning"); break;
      case ScanPhase::kPauseRequested: phase = QObject::tr("Pause requested"); break;
      case ScanPhase::kPausing: phase = QObject::tr("Pausing"); break;
      case ScanPhase::kPaused: phase = QObject::tr("Paused"); break;
      case ScanPhase::kResuming: phase = QObject::tr("Resuming"); break;
      case ScanPhase::kStopping: phase = QObject::tr("Stopping"); break;
      case ScanPhase::kFailed: phase = QObject::tr("Failed"); break;
    }
    estimated_time->setText(phase);
  });

  return section;
}

}  // namespace ui
