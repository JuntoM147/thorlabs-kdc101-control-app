#include "emergency_stop.h"

#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace ui {

QWidget* CreateEmergencyStopSection(QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Emergency stop"), parent);
  auto* layout = new QVBoxLayout(section);
  auto* button = new QPushButton(QObject::tr("EMERGENCY STOP"), section);
  button->setMinimumHeight(64);
  button->setStyleSheet(
      "QPushButton { background-color: #b71c1c; color: white; "
      "font-weight: bold; border: 2px solid #7f0000; border-radius: 6px; "
      "padding: 8px; }"
      "QPushButton:disabled { background-color: #754444; color: #dddddd; }");

  button->setEnabled(false);
  layout->addWidget(button);
  auto* status = new QLabel(QObject::tr("Unavailable — hardware not connected."), section);
  status->setWordWrap(true);
  layout->addWidget(status);
  return section;
}

}  // namespace ui
