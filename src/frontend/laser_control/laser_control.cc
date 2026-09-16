#include "laser_control.h"

#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui {

QWidget* CreateLaserControlSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Laser"), parent);
  auto* layout = new QVBoxLayout(section);

  auto* status = new QLabel(QObject::tr("Disconnected"), section);
  status->setFrameShape(QFrame::StyledPanel);
  status->setAlignment(Qt::AlignCenter);
  status->setMargin(8);
  layout->addWidget(status);

  auto* connect_button = new QPushButton(QObject::tr("Connect"), section);
  connect_button->setEnabled(false);
  connect_button->setToolTip(QObject::tr("Laser backend is not implemented yet."));
  layout->addWidget(connect_button);

  auto* on_off_button = new QPushButton(QObject::tr("On / Off"), section);
  on_off_button->setCheckable(true);
  
  on_off_button->setEnabled(false);
  on_off_button->setToolTip(QObject::tr("Laser is not connected."));
  layout->addWidget(on_off_button);

  layout->addStretch();
  return section;
}

}  // namespace ui
