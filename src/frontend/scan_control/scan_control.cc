#include "scan_control.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>

namespace ui {

QWidget* CreateScanControlSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Scan control"), parent);
  section->setObjectName(QStringLiteral("scan_control"));

  auto* layout = new QVBoxLayout(section);
  layout->setSpacing(14);

  auto* progress = new QProgressBar(section);
  progress->setRange(0, 100);
  progress->setValue(0);
  progress->setMinimumHeight(36);
  progress->setAccessibleName(QObject::tr("Scan progress"));
  layout->addWidget(progress);

  auto* estimated_time = new QLabel(QObject::tr("Estimated time: —"), section);
  estimated_time->setAccessibleName(QObject::tr("Estimated scan time"));
  estimated_time->setAlignment(Qt::AlignCenter);
  estimated_time->setToolTip(QObject::tr("Available when a scan has been configured."));
  layout->addWidget(estimated_time);

  auto* buttons = new QHBoxLayout();
  buttons->setSpacing(12);

  auto* start = new QPushButton(QObject::tr("Start"), section);
  start->setIcon(section->style()->standardIcon(QStyle::SP_MediaPlay));
  start->setProperty("primary", true);
  start->setAccessibleName(QObject::tr("Start scan"));
  start->setToolTip(QObject::tr("Scan backend is not implemented yet."));
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

  layout->addLayout(buttons);

  return section;
}

}  // namespace ui
