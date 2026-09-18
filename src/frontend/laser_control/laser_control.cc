#include "laser_control.h"

#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSizePolicy>

namespace ui {

QWidget* CreateLaserControlSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Laser control"), parent);

  auto* layout = new QVBoxLayout(section);
  layout->setSpacing(16);

  auto* connection = new QHBoxLayout();
  connection->setSpacing(12);

  auto* indicator = new QLabel(section);
  indicator->setFixedSize(12, 12);
  indicator->setProperty("connected", false);
  indicator->setStyleSheet(
      "QLabel[connected=\"false\"] { background: #ed1735; border-radius: 6px; }"
      "QLabel[connected=\"true\"] { background: #00a34a; border-radius: 6px; }");

  auto* connection_status = new QLabel(QObject::tr("Disconnected"), section);
  connection_status->setAccessibleName(QObject::tr("Laser connection status"));
  connection->addWidget(indicator);
  connection->addWidget(connection_status);
  connection->addStretch();
  layout->addLayout(connection);

  auto* connect_button = new QPushButton(QObject::tr("Connect"), section);
  connect_button->setProperty("primary", true);
  connect_button->setMinimumWidth(120);
  connect_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  connect_button->setAccessibleName(QObject::tr("Connect laser"));
  connect_button->setToolTip(QObject::tr("Laser hardware connection is not implemented yet."));
  layout->addWidget(connect_button);

  auto* status = new QPushButton(QObject::tr("Laser OFF"), section);
  status->setObjectName(QStringLiteral("laserStatus"));
  status->setAccessibleName(QObject::tr("Laser output status"));
  status->setCheckable(true);
  status->setMinimumWidth(120);
  status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  status->setToolTip(QObject::tr("UI toggle only; laser hardware control is not implemented yet."));
  layout->addWidget(status);

  QObject::connect(status, &QPushButton::toggled, status, [status](bool on) {
    status->setText(on ? QObject::tr("Laser ON") : QObject::tr("Laser OFF"));
  });

  auto* pixel_row = new QHBoxLayout();
  auto* pixel_label = new QLabel(QObject::tr("Pixel size (\u00b5m)"), section);

  auto* pixel_size = new QDoubleSpinBox(section);
  pixel_size->setMinimumWidth(100);
  pixel_size->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  pixel_size->setDecimals(3);
  pixel_size->setRange(0.001, 1000000.0);
  pixel_size->setSingleStep(0.01);
  pixel_size->setValue(0.1);
  pixel_size->setAccessibleName(QObject::tr("Pixel size in micrometres"));
  pixel_label->setBuddy(pixel_size);
  pixel_row->addWidget(pixel_label);
  pixel_row->addWidget(pixel_size, 1);
  layout->addLayout(pixel_row);

  layout->addStretch();

  return section;
}

}  // namespace ui
