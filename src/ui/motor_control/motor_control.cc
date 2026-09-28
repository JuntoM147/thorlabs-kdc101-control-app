#include "motor_control.h"
#include "main_window/display_text.h"

#include <initializer_list>
#include <memory>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDoubleValidator>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSizePolicy>

namespace ui {
namespace {

QPushButton* CreateCommandButton(const QString& text, QWidget* parent) {
  auto* button = new QPushButton(text, parent);
  button->setMinimumWidth(88);
  button->setFixedHeight(28);
  button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  button->setEnabled(false);
  button->setToolTip(QObject::tr("Motor is not connected."));

  return button;
}

QWidget* CreateAxisControl(MainWindow& view, application::Axis axis_id, const QString& axis, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("%1 Axis").arg(axis), parent);
  section->setObjectName(QStringLiteral("axis%1").arg(axis));

  auto* layout = new QVBoxLayout(section);
  layout->setContentsMargins(10, 4, 10, 4);
  layout->setSpacing(4);

  auto* header = new QHBoxLayout();
  header->setSpacing(6);
  header->addStretch();
  auto* indicator = new QLabel(section);
  indicator->setFixedSize(8, 8);
  indicator->setStyleSheet("background: #ed1735; border-radius: 4px;");
  header->addWidget(indicator);
  auto* status = new QLabel(QObject::tr("Disconnected"), section);
  status->setAccessibleName(QObject::tr("%1 axis connection status").arg(axis));
  status->setStyleSheet("color: #52627c; font-size: 12px;");
  header->addWidget(status);
  layout->addLayout(header);

  auto* body = new QHBoxLayout();
  body->setSpacing(12);
  layout->addLayout(body, 1);
  auto* manual = new QVBoxLayout();
  manual->setSpacing(4);
  body->addLayout(manual, 4);
  auto* position_label = new QLabel(QObject::tr("Current position"), section);
  position_label->setObjectName(QStringLiteral("axisFieldLabel"));
  manual->addWidget(position_label);
  auto* readout_widget = new QWidget(section);
  readout_widget->setFixedHeight(28);
  readout_widget->setObjectName(QStringLiteral("axisReadout"));
  readout_widget->setStyleSheet("QWidget#axisReadout { background: transparent; }");
  auto* readout = new QHBoxLayout(readout_widget);
  readout->setContentsMargins(0, 0, 0, 0);
  readout->setSpacing(6);
  auto* position = new QLabel(QStringLiteral("--"), section);
  position->setAccessibleName(QObject::tr("%1 axis current position").arg(axis));
  position->setStyleSheet("font-size: 21px; font-weight: 600;");
  position->setToolTip(QObject::tr("Position is unavailable until the motor is connected."));
  readout->addWidget(position);
  readout->addWidget(new QLabel(QObject::tr("mm"), section), 0, Qt::AlignBottom);
  readout->addStretch();
  manual->addWidget(readout_widget);
  manual->addStretch();

  auto* jog_down = CreateCommandButton(QStringLiteral("<"), section);
  auto* jog_up = CreateCommandButton(QStringLiteral(">"), section);
  auto* drive_down = CreateCommandButton(QStringLiteral("<<"), section);
  auto* drive_up = CreateCommandButton(QStringLiteral(">>"), section);
  jog_up->setAccessibleName(QObject::tr("%1 axis jog up").arg(axis));
  jog_down->setAccessibleName(QObject::tr("%1 axis jog down").arg(axis));
  drive_up->setAccessibleName(QObject::tr("%1 axis continuous move up").arg(axis));
  drive_down->setAccessibleName(QObject::tr("%1 axis continuous move down").arg(axis));
  for (auto* button : {jog_down, jog_up, drive_down, drive_up}) {
    button->setMinimumWidth(36);
  }
  auto* motion = new QGridLayout();
  motion->setHorizontalSpacing(6);
  motion->setVerticalSpacing(6);
  auto* jog_label = new QLabel(QObject::tr("Jog"), section);
  auto* drive_label = new QLabel(QObject::tr("Drive"), section);
  jog_label->setObjectName(QStringLiteral("axisFieldLabel"));
  drive_label->setObjectName(QStringLiteral("axisFieldLabel"));
  motion->addWidget(jog_label, 0, 0, 1, 2);
  motion->addWidget(drive_label, 0, 3, 1, 2);
  motion->setColumnMinimumWidth(2, 8);
  motion->addWidget(jog_down, 1, 0);
  motion->addWidget(jog_up, 1, 1);
  motion->addWidget(drive_down, 1, 3);
  motion->addWidget(drive_up, 1, 4);
  manual->addLayout(motion);
  manual->addSpacing(4);
  auto* actions = new QHBoxLayout();
  actions->setSpacing(8);
  auto* home = CreateCommandButton(QObject::tr("Home"), section);
  home->setMinimumWidth(72);
  home->setAccessibleName(QObject::tr("Home %1 axis").arg(axis));
  auto* stop = CreateCommandButton(QObject::tr("\u25a0 Stop"), section);
  stop->setMinimumWidth(72);
  stop->setObjectName(QStringLiteral("axisStop"));
  stop->setAccessibleName(QObject::tr("Stop %1 axis").arg(axis));
  actions->addWidget(home);
  actions->addWidget(stop);
  manual->addLayout(actions);

  auto* divider = new QFrame(section);
  divider->setFixedWidth(1);
  divider->setStyleSheet("background: #e1e5eb;");
  body->addWidget(divider);
  auto* inputs = new QGridLayout();
  inputs->setHorizontalSpacing(8);
  inputs->setVerticalSpacing(4);
  inputs->setColumnStretch(0, 1);
  body->addLayout(inputs, 5);
  auto add_field_label = [section, inputs](const QString& text, int row) {
    auto* label = new QLabel(text, section);
    label->setObjectName(QStringLiteral("axisFieldLabel"));
    inputs->addWidget(label, row, 0, 1, 3);
  };
  add_field_label(QObject::tr("Move to"), 0);

  auto* absolute_position = new QDoubleSpinBox(section);
  absolute_position->setDecimals(3);
  absolute_position->setRange(-1000000.0, 1000000.0);
  absolute_position->setButtonSymbols(QAbstractSpinBox::NoButtons);
  absolute_position->setMinimumWidth(84);
  absolute_position->setFixedHeight(28);
  absolute_position->setAlignment(Qt::AlignRight);
  absolute_position->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  absolute_position->setEnabled(false);
  absolute_position->setAccessibleName(QObject::tr("%1 axis absolute position").arg(axis));
  absolute_position->setToolTip(QObject::tr("Units and travel limits require a configured stage."));
  inputs->addWidget(absolute_position, 1, 0);
  inputs->addWidget(new QLabel(QObject::tr("mm"), section), 1, 1);

  auto* go_button = CreateCommandButton(QObject::tr("Move"), section);
  go_button->setProperty("primary", true);
  inputs->addWidget(go_button, 1, 2);

  add_field_label(QObject::tr("Step size"), 2);

  auto* step_size = new QComboBox(section);
  step_size->setEditable(true);
  step_size->addItems({QString::number(application::kDefaultJogStepMm), QStringLiteral("0.1"), QStringLiteral("1.0")});
  step_size->setInsertPolicy(QComboBox::NoInsert);

  auto* validator = new QDoubleValidator(0.000001, 1000000.0, 6, step_size);
  validator->setNotation(QDoubleValidator::StandardNotation);
  validator->setLocale(QLocale::c());
  step_size->setValidator(validator);
  step_size->setMinimumWidth(84);
  step_size->setFixedHeight(28);
  step_size->lineEdit()->setAlignment(Qt::AlignRight);
  step_size->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  step_size->setAccessibleName(QObject::tr("%1 axis jog step size in mm").arg(axis));
  step_size->setToolTip(QObject::tr("Choose a preset or type a positive step size in mm."));
  step_size->setStyleSheet(
      "QComboBox { background: white; border: 1px solid #cfd8e5; border-radius: 4px; "
      "padding: 2px 5px; min-height: 20px; }"
      "QComboBox:focus { border-color: #126bf0; }"
      "QComboBox QLineEdit { border: none; padding: 0; min-height: 0; }");
  inputs->addWidget(step_size, 3, 0);
  inputs->addWidget(new QLabel(QObject::tr("mm"), section), 3, 1);
  step_size->setEnabled(false);

  auto* apply_step = CreateCommandButton(QObject::tr("Apply"), section);
  apply_step->setAccessibleName(QObject::tr("Apply %1 axis jog step").arg(axis));
  inputs->addWidget(apply_step, 3, 2);
  step_size->setToolTip(QObject::tr("Choose a step in mm, then click Apply. Jog uses the applied device settings."));
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
  add_field_label(QObject::tr("Speed"), 4);
  auto* speed = new QLineEdit(QString::number(application::kDefaultSpeedMmPerSecond), section);
  auto* speed_validator = new QDoubleValidator(0.000001, 1000000.0, 6, speed);
  speed_validator->setNotation(QDoubleValidator::StandardNotation);
  speed_validator->setLocale(QLocale::c());
  speed->setValidator(speed_validator);
  speed->setMinimumWidth(84);
  speed->setFixedHeight(28);
  speed->setAlignment(Qt::AlignRight);
  speed->setEnabled(false);
  speed->setAccessibleName(QObject::tr("%1 axis speed in mm per second").arg(axis));
  speed->setToolTip(QObject::tr("Apply speed to moves, jogging and homing. Acceleration is unchanged."));
  inputs->addWidget(speed, 5, 0);
  inputs->addWidget(new QLabel(QObject::tr("mm/s"), section), 5, 1);
  auto* apply_speed = CreateCommandButton(QObject::tr("Apply"), section);
  apply_speed->setAccessibleName(QObject::tr("Apply %1 axis speed").arg(axis));
  inputs->addWidget(apply_speed, 5, 2);
  QObject::connect(apply_speed, &QPushButton::clicked, section, [&view, axis_id, speed] {
    bool ok = false;
    const double value = QLocale::c().toDouble(speed->text(), &ok);
    if (!ok || !speed->hasAcceptableInput() || value <= 0) {
      view.ShowError({axis_id, "Configure speed", "Enter a positive speed in mm/s."}); return;
    }
    application::MotorSettings settings;
    settings.move.speed_mm_per_second = value;
    settings.jog.speed_mm_per_second = value;
    settings.homing_speed_mm_per_second = value;
    emit view.ConfigureAxisRequested(axis_id, settings);
  });
  for (auto* button : {go_button, apply_step, apply_speed}) {
    button->setFixedWidth(94);
  }
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
  for (auto* button : {jog_up, jog_down, drive_up, drive_down, home, stop, go_button, apply_step, apply_speed}) {
    button->setToolTip(QString());
  }
  jog_down->setToolTip(QObject::tr("Jog one applied step in the negative direction."));
  jog_up->setToolTip(QObject::tr("Jog one applied step in the positive direction."));
  drive_down->setToolTip(QObject::tr("Move continuously in the negative direction until stopped."));
  drive_up->setToolTip(QObject::tr("Move continuously in the positive direction until stopped."));
  struct Availability {
    application::AxisState state{};
    bool pending = false;
    bool stopping = false;
  };
  const auto availability = std::make_shared<Availability>();
  const auto refresh = [=] {
    const auto& state = availability->state;
    const bool connected = state.connection == application::ConnectionState::kConnected;
    const bool ready = connected && state.operation == application::OperationState::kIdle && !availability->pending;
    for (auto* button : {jog_up, jog_down, drive_up, drive_down, home, go_button, apply_step, apply_speed}) {
      button->setEnabled(ready);
    }
    stop->setEnabled(connected && !availability->stopping && state.operation != application::OperationState::kStopping);
    absolute_position->setEnabled(ready);
    step_size->setEnabled(ready);
    speed->setEnabled(ready);
  };
  QObject::connect(&view, &MainWindow::AxisRequestsPending, section,
                   [=](application::Axis axis, bool pending, bool stopping) {
    if (axis != axis_id) return;
    availability->pending = pending;
    availability->stopping = stopping;
    refresh();
  });
  QObject::connect(&view, &MainWindow::AxisStateUpdated, section, [=](application::AxisState state) {
    if (state.axis != axis_id) return;
    if (state.connection == application::ConnectionState::kConnected &&
        availability->state.connection != application::ConnectionState::kConnected) {
      step_size->setCurrentText(QString::number(application::kDefaultJogStepMm));
      speed->setText(QString::number(application::kDefaultSpeedMmPerSecond));
    }
    availability->state = state;
    refresh();
    const bool connected = state.connection == application::ConnectionState::kConnected;
    position->setText(connected && state.position_mm ? QString::number(*state.position_mm, 'f', 3) : QStringLiteral("--"));
    position->setToolTip(QString());
    status->setText(ConnectionText(state.connection) + " / " + OperationText(state.operation));
    indicator->setStyleSheet(connected ? "background: #00a34a; border-radius: 4px;"
                                      : "background: #ed1735; border-radius: 4px;");
  });
  return section;
}
}  // namespace

QWidget* CreateMotorControlSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Motor control"), parent);
  section->setObjectName(QStringLiteral("motorControls"));

  auto* layout = new QVBoxLayout(section);
  layout->setContentsMargins(6, 4, 6, 4);
  layout->setSpacing(6);
  int axis_index = 0;
  for (const auto& axis : {QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")}) {
    layout->addWidget(CreateAxisControl(view, static_cast<application::Axis>(axis_index), axis, section));
    ++axis_index;
  }

  section->setEnabled(false);
  QObject::connect(&view, &MainWindow::ManualControlsEnabled, section, &QWidget::setEnabled);

  return section;
}

}  // namespace ui
