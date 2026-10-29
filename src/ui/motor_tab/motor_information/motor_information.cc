#include "motor_information.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QSizePolicy>
#include <array>
#include <initializer_list>
#include <memory>

#include "shared/display_text.h"

namespace ui {

QWidget* CreateMotorInformationSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Motor Information"), parent);
  section->setObjectName(QStringLiteral("motor_information"));

  auto* layout = new QGridLayout(section);
  layout->setHorizontalSpacing(16);
  layout->setVerticalSpacing(6);
  layout->setColumnStretch(3, 1);
  layout->setColumnStretch(4, 2);
  layout->setColumnStretch(5, 1);

  auto channels = std::make_shared<std::array<QComboBox*, 3>>();
  layout->addWidget(new QLabel(QObject::tr("Controller serial"), section), 0, 0,
                    1, 2);
  auto* controller_serial = new QLineEdit(QStringLiteral("70871422"), section);
  controller_serial->setObjectName("controllerSerial");
  controller_serial->setAccessibleName(
      QObject::tr("Shared controller serial number"));
  controller_serial->setValidator(new QRegularExpressionValidator(
      QRegularExpression(QStringLiteral("[0-9]{8}")), controller_serial));
  layout->addWidget(controller_serial, 0, 2, 1, 3);
  auto pending = std::make_shared<std::array<bool, 3>>();
  auto connected = std::make_shared<std::array<bool, 3>>();
  const auto refresh_mapping = [=] {
    bool locked = false;
    for (int i = 0; i < 3; ++i) locked |= (*pending)[i] || (*connected)[i];
    controller_serial->setEnabled(!locked);
    for (auto* channel : *channels)
      if (channel) channel->setEnabled(!locked);
  };
  QObject::connect(&view, &MainWindow::AxisRequestsPending, section,
                   [=](ui::Axis axis, bool busy, bool) {
                     (*pending)[static_cast<int>(axis)] = busy;
                     refresh_mapping();
                   });
  QObject::connect(&view, &MainWindow::AxisStateUpdated, section,
                   [=](ui::AxisState state) {
                     (*connected)[static_cast<int>(state.axis)] =
                         state.connection != ui::ConnectionState::kDisconnected;
                     refresh_mapping();
                   });

  int axis_index = 0;
  for (const auto& axis :
       {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")}) {
    const int row = axis_index + 1;
    layout->addWidget(new QLabel(QObject::tr("Axis %1").arg(axis), section),
                      row, 0);

    auto* indicator = new QLabel(section);
    indicator->setObjectName(QStringLiteral("connectionIndicator%1").arg(axis));
    indicator->setFixedSize(12, 12);
    indicator->setProperty("connected", false);
    indicator->setAccessibleName(QObject::tr("%1 axis disconnected").arg(axis));
    indicator->setStyleSheet(
        "QLabel[connected=\"false\"] { background: #ed1735; border-radius: "
        "6px; }"
        "QLabel[connected=\"true\"] { background: #00a34a; border-radius: 6px; "
        "}");
    layout->addWidget(indicator, row, 1, Qt::AlignCenter);
    auto* connection_status = new QLabel(QObject::tr("Disconnected"), section);
    layout->addWidget(connection_status, row, 2);

    auto* serial_number = controller_serial;
    auto* channel = new QComboBox(section);
    channel->setObjectName(QStringLiteral("axisChannel%1").arg(axis));
    channel->setAccessibleName(
        QObject::tr("%1 axis controller channel").arg(axis));
    channel->addItem(QObject::tr("Select channel"), 0);
    for (int ch = 1; ch <= 3; ++ch)
      channel->addItem(QObject::tr("Channel %1").arg(ch), ch);
    channel->setCurrentIndex(0);
    (*channels)[axis_index] = channel;
    layout->addWidget(channel, row, 4);

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
    homing_status->setAccessibleName(
        QObject::tr("%1 axis homing status").arg(axis));
    homing_status->setToolTip(QObject::tr(
        "Default status; homing has not been confirmed by hardware."));
    homing_status->setMinimumWidth(80);
    homing_status->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    layout->addWidget(homing_status, row, 3);

    const auto axis_id = static_cast<ui::Axis>(axis_index);
    QObject::connect(
        connect_button, &QPushButton::clicked, section,
        [&view, axis_id, connect_button, serial_number, channels] {
          if (connect_button->property("connected").toBool()) {
            emit view.DisconnectMotorRequested(axis_id);
          } else {
            const auto serial = serial_number->text().trimmed();
            if (serial.isEmpty()) {
              view.ShowError(
                  {axis_id, "Connect", "Enter a motor serial number."});
              return;
            }
            if (!serial_number->hasAcceptableInput()) {
              view.ShowError({axis_id, "Connect",
                              "Enter the eight-digit controller serial "
                              "without a channel suffix."});
              return;
            }
            bool used[4] = {};
            for (auto* selector : *channels) {
              const int ch = selector->currentData().toInt();
              if (ch < 1 || ch > 3 || used[ch]) {
                view.ShowError({axis_id, "Connect",
                                "Assign a different channel (1, 2 or 3) to "
                                "each axis before connecting."});
                return;
              }
              used[ch] = true;
            }
            const short selected_channel = static_cast<short>(
                (*channels)[static_cast<int>(axis_id)]->currentData().toInt());

            emit view.ConnectMotorRequested(
                {axis_id, serial.toStdString(), false, selected_channel});
          }
        });
    struct Availability {
      ui::AxisState state{};
      bool pending = false;
    };
    const auto availability = std::make_shared<Availability>();
    const auto refresh = [=] {
      const auto& state = availability->state;
      const bool connected =
          state.connection == ui::ConnectionState::kConnected;
      const bool ready =
          !availability->pending &&
          state.connection != ui::ConnectionState::kConnecting &&
          (!connected || state.operation == ui::OperationState::kIdle);
      connect_button->setEnabled(ready);
    };
    QObject::connect(&view, &MainWindow::AxisRequestsPending, section,
                     [=](ui::Axis axis, bool pending, bool) {
                       if (axis != axis_id) return;
                       availability->pending = pending;
                       refresh();
                     });
    QObject::connect(
        &view, &MainWindow::AxisStateUpdated, section,
        [=](ui::AxisState state) {
          if (state.axis != axis_id) return;
          const bool connected =
              state.connection == ui::ConnectionState::kConnected;
          connect_button->setProperty("connected", connected);
          connect_button->setText(connected ? QObject::tr("Disconnect")
                                            : QObject::tr("Connect"));
          availability->state = state;
          refresh();
          connection_status->setText(ConnectionText(state.connection));
          homing_status->setText(!state.homed   ? QObject::tr("Unknown")
                                 : *state.homed ? QObject::tr("Homed")
                                                : QObject::tr("Unhomed"));
          homing_status->setToolTip(QString());
          indicator->setAccessibleName(
              QObject::tr("%1 axis %2")
                  .arg(axis, ConnectionText(state.connection)));
          indicator->setStyleSheet(
              connected ? "background: #00a34a; border-radius: 6px;"
                        : "background: #ed1735; border-radius: 6px;");
        });
    ++axis_index;
  }

  section->setEnabled(false);
  QObject::connect(&view, &MainWindow::RecoveryControlsEnabled, section,
                   &QWidget::setEnabled);
  return section;
}

}  // namespace ui
