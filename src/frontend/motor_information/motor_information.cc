#include "motor_information.h"

#include <initializer_list>

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>

namespace ui {

QWidget* CreateMotorInformationSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Motor information"), parent);
  section->setObjectName(QStringLiteral("motor_information"));

  auto* layout = new QGridLayout(section);
  layout->setHorizontalSpacing(16);
  layout->setVerticalSpacing(14);
  layout->setColumnStretch(3, 1);
  layout->setColumnStretch(4, 2);
  layout->setColumnStretch(5, 1);

  int row = 0;
  for (const auto& axis : {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")}) {
    layout->addWidget(new QLabel(QObject::tr("Axis %1").arg(axis), section), row, 0);

    auto* indicator = new QLabel(section);
    indicator->setObjectName(QStringLiteral("connectionIndicator%1").arg(axis));
    indicator->setFixedSize(12, 12);
    indicator->setProperty("connected", false);
    indicator->setAccessibleName(QObject::tr("%1 axis disconnected").arg(axis));
    indicator->setStyleSheet(
        "QLabel[connected=\"false\"] { background: #ed1735; border-radius: 6px; }"
        "QLabel[connected=\"true\"] { background: #00a34a; border-radius: 6px; }");
    layout->addWidget(indicator, row, 1, Qt::AlignCenter);
    layout->addWidget(new QLabel(QObject::tr("Disconnected"), section), row, 2);

    auto* serial_number = new QLineEdit(QString::number(27000001 + row), section);
    serial_number->setPlaceholderText(QObject::tr("Serial number"));
    serial_number->setAccessibleName(QObject::tr("%1 axis serial number").arg(axis));
    serial_number->setMinimumWidth(100);
    serial_number->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    layout->addWidget(serial_number, row, 4);

    auto* connect_button = new QPushButton(QObject::tr("Connect"), section);
    connect_button->setProperty("primary", true);
    connect_button->setMinimumWidth(88);
    connect_button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect_button->setAccessibleName(QObject::tr("Connect %1 axis").arg(axis));
    connect_button->setEnabled(true);
    connect_button->setToolTip(QObject::tr("Motor backend is not implemented yet."));
    layout->addWidget(connect_button, row, 5);

    auto* homing_status = new QLineEdit(QObject::tr("Unhomed"), section);
    homing_status->setReadOnly(true);
    homing_status->setAccessibleName(QObject::tr("%1 axis homing status").arg(axis));
    homing_status->setToolTip(QObject::tr("Default status; homing has not been confirmed by hardware."));
    homing_status->setMinimumWidth(80);
    homing_status->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    layout->addWidget(homing_status, row, 3);

    ++row;
  }

  return section;
}

}  // namespace ui
