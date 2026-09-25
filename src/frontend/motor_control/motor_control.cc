#include "motor_control.h"
#include "main_window/display_text.h"

#include <initializer_list>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QStyle>

namespace ui {
namespace {

QPushButton* CreateCommandButton(const QString& text, QWidget* parent) {
  auto* button = new QPushButton(text, parent);
  button->setMinimumSize(88, 40);
  button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  button->setEnabled(false);
  button->setToolTip(QObject::tr("Motor is not connected."));

  return button;
}

QWidget* CreateAxisControl(MainWindow& view, application::Axis axis_id, const QString& axis, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("%1 Axis").arg(axis), parent);
  section->setObjectName(QStringLiteral("axis%1").arg(axis));

  auto* layout = new QVBoxLayout(section);
  layout->setSpacing(12);

  auto* connection = new QHBoxLayout();

  auto* indicator = new QLabel(section);
  indicator->setFixedSize(10, 10);
  indicator->setStyleSheet("background: #ed1735; border-radius: 5px;");
  connection->addWidget(indicator);
  connection->addSpacing(6);

  auto* status = new QLabel(QObject::tr("DISCONNECTED"), section);
  status->setAccessibleName(QObject::tr("%1 axis connection status").arg(axis));
  status->setStyleSheet("color: #52627c; font-size: 12px;");
  connection->addWidget(status);
  connection->addStretch();

  auto* controls = new QHBoxLayout();
  controls->setSpacing(10);

  auto* position = new QLabel(QObject::tr("Unknown"), section);
  position->setAccessibleName(QObject::tr("%1 axis current position").arg(axis));
  position->setStyleSheet("font-size: 24px; font-weight: 600;");
  position->setToolTip(QObject::tr("Position is unavailable until the motor is connected."));
  controls->addWidget(position);
  controls->addWidget(new QLabel(QObject::tr("mm"), section));
  controls->addStretch();

  auto* command_grid = new QGridLayout();
  command_grid->setHorizontalSpacing(8);
  command_grid->setVerticalSpacing(12);
  // The position/move column gets twice the width of each command column.
  command_grid->setColumnStretch(0, 2);
  for (int column = 1; column <= 3; ++column) {
    command_grid->setColumnStretch(column, 1);
    command_grid->setColumnMinimumWidth(column, 88);
  }

  command_grid->addLayout(connection, 0, 0, Qt::AlignVCenter);
  command_grid->addLayout(controls, 1, 0, Qt::AlignVCenter);

  auto* jog_up = CreateCommandButton(QString(), section);
  jog_up->setIcon(section->style()->standardIcon(QStyle::SP_ArrowUp));
  jog_up->setIconSize(QSize(20, 20));
  jog_up->setAccessibleName(QObject::tr("%1 axis jog up").arg(axis));
  command_grid->addWidget(jog_up, 0, 1);

  auto* jog_down = CreateCommandButton(QString(), section);
  jog_down->setIcon(section->style()->standardIcon(QStyle::SP_ArrowDown));
  jog_down->setIconSize(QSize(20, 20));
  jog_down->setAccessibleName(QObject::tr("%1 axis jog down").arg(axis));
  command_grid->addWidget(jog_down, 1, 1);

  auto* drive_up = CreateCommandButton(QStringLiteral("\u25b2"), section);
  drive_up->setAccessibleName(QObject::tr("%1 axis continuous move up").arg(axis));
  command_grid->addWidget(drive_up, 0, 2);

  auto* drive_down = CreateCommandButton(QStringLiteral("\u25bc"), section);
  drive_down->setAccessibleName(QObject::tr("%1 axis continuous move down").arg(axis));
  command_grid->addWidget(drive_down, 1, 2);

  auto* home = CreateCommandButton(QObject::tr("Home"), section);
  home->setAccessibleName(QObject::tr("Home %1 axis").arg(axis));
  command_grid->addWidget(home, 0, 3);

  auto* stop = CreateCommandButton(QObject::tr("Stop"), section);
  stop->setIcon(section->style()->standardIcon(QStyle::SP_MediaStop));
  stop->setIconSize(QSize(16, 16));
  stop->setAccessibleName(QObject::tr("Stop %1 axis").arg(axis));
  command_grid->addWidget(stop, 1, 3);

  layout->addLayout(command_grid);

  auto* inputs = new QHBoxLayout();
  inputs->setSpacing(10);
  inputs->addWidget(new QLabel(QObject::tr("Move to"), section));

  auto* absolute_position = new QDoubleSpinBox(section);
  absolute_position->setDecimals(3);
  absolute_position->setRange(-1000000.0, 1000000.0);
  absolute_position->setButtonSymbols(QAbstractSpinBox::NoButtons);
  absolute_position->setMinimumWidth(70);
  absolute_position->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  absolute_position->setEnabled(false);
  absolute_position->setAccessibleName(QObject::tr("%1 axis absolute position").arg(axis));
  absolute_position->setToolTip(QObject::tr("Units and travel limits require a configured stage."));
  inputs->addWidget(absolute_position, 1);
  inputs->addWidget(new QLabel(QObject::tr("mm"), section));

  auto* go_button = CreateCommandButton(QObject::tr("Go"), section);
  command_grid->addWidget(go_button, 2, 1);

  command_grid->addWidget(new QLabel(QObject::tr("Step (mm):"), section), 2, 2, Qt::AlignRight | Qt::AlignVCenter);

  auto* step_size = new QComboBox(section);
  step_size->setEditable(true);
  step_size->addItems({QStringLiteral("0.005"), QStringLiteral("0.1"), QStringLiteral("1.0")});
  step_size->setInsertPolicy(QComboBox::NoInsert);

  auto* validator = new QDoubleValidator(0.000001, 1000000.0, 6, step_size);
  validator->setNotation(QDoubleValidator::StandardNotation);
  validator->setLocale(QLocale::c());
  step_size->setValidator(validator);
  step_size->setMinimumWidth(88);
  step_size->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  step_size->setAccessibleName(QObject::tr("%1 axis jog step size in mm").arg(axis));
  step_size->setToolTip(QObject::tr("Choose a preset or type a positive step size in mm."));
  step_size->setStyleSheet(
      "QComboBox { background: white; border: 1px solid #cfd8e5; border-radius: 4px; "
      "padding: 5px; min-height: 20px; }"
      "QComboBox:focus { border-color: #126bf0; }"
      "QComboBox QLineEdit { border: none; padding: 0; min-height: 0; }");
  command_grid->addWidget(step_size, 2, 3);
  command_grid->addLayout(inputs, 2, 0);

  auto* apply_step = CreateCommandButton(QObject::tr("Apply step"), section);
  apply_step->setAccessibleName(QObject::tr("Apply %1 axis jog step").arg(axis));
  command_grid->addWidget(apply_step, 3, 3);
  step_size->setToolTip(QObject::tr("Choose a step in mm, then click Apply step. Jog uses the applied device settings."));
  QObject::connect(apply_step, &QPushButton::clicked, section, [&view, axis_id, step_size] {
    bool ok = false;
    const double step = QLocale::c().toDouble(step_size->currentText(), &ok);
    if (!ok || !step_size->lineEdit()->hasAcceptableInput() || step <= 0) {
      view.ShowError({axis_id, "Configure jog", "Enter a positive step size in mm."}); return;
    }
    application::MotorSettings settings;
    settings.jog_step_mm = step;
    settings.jog_mode = application::JogMode::kSingleStep;
    emit view.ConfigureAxisRequested(axis_id, settings);
  });
  auto jog = [&view, axis_id](application::Direction direction) {
    emit view.JogAxisRequested(axis_id, direction);
  };
  QObject::connect(jog_up, &QPushButton::clicked, section, [jog] { jog(application::Direction::kForward); });
  QObject::connect(jog_down, &QPushButton::clicked, section, [jog] { jog(application::Direction::kBackward); });
  QObject::connect(drive_up, &QPushButton::clicked, section, [&view, axis_id] {
    emit view.DriveAxisRequested(axis_id, application::Direction::kForward);
  });
  QObject::connect(drive_down, &QPushButton::clicked, section, [&view, axis_id] {
    emit view.DriveAxisRequested(axis_id, application::Direction::kBackward);
  });
  QObject::connect(home, &QPushButton::clicked, section, [&view, axis_id] { emit view.HomeAxisRequested(axis_id); });
  QObject::connect(stop, &QPushButton::clicked, section, [&view, axis_id] {
    emit view.StopAxisRequested(axis_id, application::StopMode::kProfiled);
  });
  QObject::connect(go_button, &QPushButton::clicked, section, [&view, axis_id, absolute_position] {
    emit view.MoveAxisRequested(axis_id, absolute_position->value());
  });
  go_button->setAccessibleName(QObject::tr("Move %1 axis").arg(axis));
  for (auto* button : {jog_up, jog_down, drive_up, drive_down, home, stop, go_button, apply_step}) {
    button->setToolTip(QString());
  }
  QObject::connect(&view, &MainWindow::AxisStateUpdated, section, [=](application::AxisState state) {
    if (state.axis != axis_id) return;
    const bool connected = state.connection == application::ConnectionState::kConnected;
    const bool idle = state.operation == application::OperationState::kIdle;
    for (auto* button : {jog_up, jog_down, drive_up, drive_down, home, go_button, apply_step}) {
      button->setEnabled(connected && idle);
    }
    stop->setEnabled(connected);
    absolute_position->setEnabled(connected && idle);
    step_size->setEnabled(connected && idle);
    position->setText(state.position_mm ? QString::number(*state.position_mm, 'f', 3) : QObject::tr("Unknown"));
    position->setToolTip(QString());
    status->setText(ConnectionText(state.connection) + " / " + OperationText(state.operation));
    indicator->setStyleSheet(connected ? "background: #00a34a; border-radius: 5px;"
                                      : "background: #ed1735; border-radius: 5px;");
  });
  return section;
}
}  // namespace

QWidget* CreateMotorControlSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Motor control"), parent);

  auto* layout = new QVBoxLayout(section);
  layout->setSpacing(12);
  int axis_index = 0;
  for (const auto& axis : {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")}) {
    layout->addWidget(CreateAxisControl(view, static_cast<application::Axis>(axis_index), axis, section));
    ++axis_index;
  }

  layout->addStretch();
  section->setEnabled(false);
  QObject::connect(&view, &MainWindow::ManualControlsEnabled, section, &QWidget::setEnabled);

  return section;
}

}  // namespace ui
