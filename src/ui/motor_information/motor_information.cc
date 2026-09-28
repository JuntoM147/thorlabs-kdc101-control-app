#include "motor_information.h"
#include "main_window/display_text.h"

#include <initializer_list>
#include <array>
#include <memory>

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>

namespace ui {

QWidget* CreateMotorInformationSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Motor information"), parent);
  section->setObjectName(QStringLiteral("motor_information"));

  auto* layout = new QGridLayout(section);
  layout->setHorizontalSpacing(16);
  layout->setVerticalSpacing(6);
  layout->setColumnStretch(3, 1);
  layout->setColumnStretch(4, 2);
  layout->setColumnStretch(5, 1);

  constexpr std::array<int, 3> default_serial_numbers{27267150, 27266365, 27266180};
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
    auto* connection_status = new QLabel(QObject::tr("Disconnected"), section);
    layout->addWidget(connection_status, row, 2);

    auto* serial_number = new QLineEdit(QString::number(default_serial_numbers[row]), section);
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
    connect_button->setProperty("connected", false);
    layout->addWidget(connect_button, row, 5);

    auto* homing_status = new QLineEdit(QObject::tr("Unknown"), section);
    homing_status->setReadOnly(true);
    homing_status->setAccessibleName(QObject::tr("%1 axis homing status").arg(axis));
    homing_status->setToolTip(QObject::tr("Default status; homing has not been confirmed by hardware."));
    homing_status->setMinimumWidth(80);
    homing_status->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    layout->addWidget(homing_status, row, 3);

    const auto axis_id = static_cast<application::Axis>(row);
    QObject::connect(connect_button, &QPushButton::clicked, section, [&view, axis_id, connect_button, serial_number] {
      if (connect_button->property("connected").toBool()) {
        emit view.DisconnectMotorRequested(axis_id);
      } else {
        const auto serial = serial_number->text().trimmed();
        if (serial.isEmpty()) {
          view.ShowError({axis_id, "Connect", "Enter a motor serial number."});
          return;
        }
        emit view.ConnectMotorRequested({axis_id, serial.toStdString()});
      }
    });
    struct Availability {
      application::AxisState state{};
      bool pending = false;
    };
    const auto availability = std::make_shared<Availability>();
    const auto refresh = [=] {
      const auto& state = availability->state;
      const bool connected = state.connection == application::ConnectionState::kConnected;
      const bool ready = !availability->pending && state.connection != application::ConnectionState::kConnecting
          && (!connected || state.operation == application::OperationState::kIdle);
      connect_button->setEnabled(ready);
      serial_number->setEnabled(ready && !connected);
    };
    QObject::connect(&view, &MainWindow::AxisRequestsPending, section,
                     [=](application::Axis axis, bool pending, bool) {
      if (axis != axis_id) return;
      availability->pending = pending;
      refresh();
    });
    QObject::connect(&view, &MainWindow::AxisStateUpdated, section, [=](application::AxisState state) {
      if (state.axis != axis_id) return;
      const bool connected = state.connection == application::ConnectionState::kConnected;
      connect_button->setProperty("connected", connected);
      connect_button->setText(connected ? QObject::tr("Disconnect") : QObject::tr("Connect"));
      availability->state = state;
      refresh();
      connection_status->setText(ConnectionText(state.connection));
      homing_status->setText(!state.homed ? QObject::tr("Unknown")
                           : *state.homed ? QObject::tr("Homed") : QObject::tr("Unhomed"));
      homing_status->setToolTip(QString());
      indicator->setAccessibleName(QObject::tr("%1 axis %2").arg(axis, ConnectionText(state.connection)));
      indicator->setStyleSheet(connected ? "background: #00a34a; border-radius: 6px;"
                                        : "background: #ed1735; border-radius: 6px;");
    });
    ++row;
  }

  section->setEnabled(false);
  QObject::connect(&view, &MainWindow::ManualControlsEnabled, section, &QWidget::setEnabled);
  return section;
}

}  // namespace ui
