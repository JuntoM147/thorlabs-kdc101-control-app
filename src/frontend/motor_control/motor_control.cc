#include "motor_control.h"

#include <initializer_list>
#include <optional>

#include <QDoubleSpinBox>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui {
namespace {

QPushButton* CreateCommandButton(const QString& text, QWidget* parent) {
  auto* button = new QPushButton(text, parent);
  button->setEnabled(false);
  button->setToolTip(QObject::tr("Motor is not connected."));
  return button;
}

QDoubleSpinBox* CreatePositionInput(QWidget* parent) {
  auto* input = new QDoubleSpinBox(parent);
  input->setDecimals(3);
  input->setEnabled(false);
  input->setToolTip(QObject::tr("Units and travel limits require a configured stage."));
  return input;
}

void SetHomingStatus(QLineEdit* status, std::optional<bool> needs_homing) {
  if (!needs_homing.has_value()) {
    status->setText(QObject::tr("Homing: unknown"));
    status->setStyleSheet(QString());
  } else if (*needs_homing) {
    status->setText(QObject::tr("Needs homing"));
    status->setStyleSheet(
        "QLineEdit { background-color: #b71c1c; color: white; }");
  } else {
    status->setText(QObject::tr("Homed"));
    status->setStyleSheet(
        "QLineEdit { background-color: #1b5e20; color: white; }");
  }
}

QWidget* CreateAxisControl(const QString& axis, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("%1 axis").arg(axis), parent);
  auto* layout = new QGridLayout(section);
  layout->setColumnStretch(2, 1);
  layout->setColumnStretch(3, 1);

  auto* serial_number = new QLineEdit(section);
  serial_number->setPlaceholderText(QObject::tr("Serial number"));
  serial_number->setAccessibleName(QObject::tr("%1 axis serial number").arg(axis));
  layout->addWidget(serial_number, 0, 0, 1, 2);
  auto* connect_button = CreateCommandButton(QObject::tr("Connect"), section);
  connect_button->setToolTip(QObject::tr("Motor backend is not implemented yet."));
  layout->addWidget(connect_button, 0, 2, 1, 2);

  auto* position = new QLineEdit(QObject::tr("Unknown"), section);
  position->setReadOnly(true);
  position->setAccessibleName(QObject::tr("Current position"));
  QFont position_font = position->font();
  if (position_font.pointSizeF() > 0) {
    position_font.setPointSizeF(position_font.pointSizeF() + 2);
  } else {
    position_font.setPixelSize(position_font.pixelSize() + 3);
  }

  position_font.setBold(true);
  position->setFont(position_font);
  layout->addWidget(new QLabel(QObject::tr("Position"), section), 1, 0);
  layout->addWidget(position, 1, 1, 1, 3);

  layout->addWidget(CreateCommandButton(QObject::tr("Home"), section), 2, 0, 1, 2);
  auto* homing_status = new QLineEdit(section);
  homing_status->setReadOnly(true);
  homing_status->setAccessibleName(QObject::tr("Needs homing"));
  homing_status->setToolTip(QObject::tr("Homing state is unavailable until the motor is connected."));
  SetHomingStatus(homing_status, std::nullopt);
  layout->addWidget(homing_status, 2, 2, 1, 2);

  layout->addWidget(new QLabel(QObject::tr("Jog"), section), 3, 0);
  auto* step_size = CreatePositionInput(section);
  step_size->setAccessibleName(QObject::tr("Jog step size"));
  step_size->setToolTip(QObject::tr("Jog step size."));
  layout->addWidget(step_size, 3, 1);
  layout->addWidget(CreateCommandButton(QObject::tr("<<<"), section), 3, 2);
  layout->addWidget(CreateCommandButton(QObject::tr(">>>"), section), 3, 3);

  layout->addWidget(new QLabel(QObject::tr("Move absolute"), section), 4, 0);
  auto* absolute_position = CreatePositionInput(section);
  absolute_position->setAccessibleName(QObject::tr("Absolute position"));
  layout->addWidget(absolute_position, 4, 1, 1, 2);
  layout->addWidget(CreateCommandButton(QObject::tr("Move"), section), 4, 3);

  layout->addWidget(new QLabel(QObject::tr("Stop"), section), 5, 0);
  layout->addWidget(CreateCommandButton(QObject::tr("Immediate"), section), 5, 2);
  layout->addWidget(CreateCommandButton(QObject::tr("Profiled"), section), 5, 3);

  return section;
}
}  // namespace

QWidget* CreateMotorControlSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Motor control"), parent);
  auto* layout = new QVBoxLayout(section);

  // X, Y, Z motors
  for (const auto& axis : {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")}) {
    layout->addWidget(CreateAxisControl(axis, section));
  }

  layout->addStretch();
  return section;
}

}  // namespace ui
