#include "options.h"
#include "main_window/main_window.h"

#include <cmath>
#include <memory>

#include <QDoubleValidator>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>

namespace ui {
QWidget* CreateOptionsSection(MainWindow& view, QWidget* parent) {
  auto* page = new QWidget(parent);
  auto* layout = new QGridLayout(page);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(16);
  for (int column = 0; column < 3; ++column) layout->setColumnStretch(column, 1);
  layout->setRowStretch(2, 1);

  auto* offset_group = new QGroupBox(QObject::tr("Offset"), page);
  auto* offset_layout = new QGridLayout(offset_group);
  auto* offset = new QLineEdit(offset_group);
  offset->setAccessibleName(QObject::tr("Offset"));
  auto* apply_offset = new QPushButton(QObject::tr("Apply"), offset_group);
  apply_offset->setAccessibleName(QObject::tr("Apply offset"));
  offset_layout->addWidget(offset, 0, 0);
  offset_layout->addWidget(apply_offset, 0, 1);
  // TODO: Define offset semantics and implement Apply; this field has no behavior yet.
  layout->addWidget(offset_group, 0, 0);

  auto* acceleration_group = new QGroupBox(QObject::tr("Motor acceleration"), page);
  auto* grid = new QGridLayout(acceleration_group);
  grid->setColumnStretch(1, 1);
  const QStringList axes{"X", "Y", "Z"};
  for (int i = 0; i < axes.size(); ++i) {
    const auto axis = static_cast<application::Axis>(i);
    const auto name = axes.at(i);
    auto* input = new QLineEdit(acceleration_group);
    input->setAccessibleName(QObject::tr("%1 axis acceleration").arg(name));
    input->setPlaceholderText(QObject::tr("Enter acceleration"));
    auto* validator = new QDoubleValidator(0.000001, 1000000.0, 6, input);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::c());
    input->setValidator(validator);
    auto* apply = new QPushButton(QObject::tr("Apply"), acceleration_group);
    apply->setAccessibleName(QObject::tr("Apply %1 axis acceleration").arg(name));
    auto* feedback = new QLabel(acceleration_group);
    feedback->setWordWrap(true);
    feedback->setAccessibleName(QObject::tr("%1 axis applied acceleration").arg(name));
    grid->addWidget(new QLabel(QObject::tr("%1 axis").arg(name), acceleration_group), i * 2, 0);
    grid->addWidget(input, i * 2, 1);
    grid->addWidget(new QLabel(QObject::tr("mm/s²"), acceleration_group), i * 2, 2);
    grid->addWidget(apply, i * 2, 3);
    grid->addWidget(feedback, i * 2 + 1, 0, 1, 4);

    struct Availability {
      application::AxisState state;
      bool pending = false;
      bool stopping = false;
    };
    auto availability = std::make_shared<Availability>();
    const auto refresh = [=] {
      const bool ready = availability->state.connection == application::ConnectionState::kConnected &&
          availability->state.operation == application::OperationState::kIdle &&
          !availability->pending && !availability->stopping;
      input->setEnabled(ready);
      apply->setEnabled(ready);
    };
    refresh();
    QObject::connect(&view, &MainWindow::AxisStateUpdated, page, [=](application::AxisState state) {
      if (state.axis != axis) return;
      availability->state = state;
      if (state.connection != application::ConnectionState::kConnected) feedback->clear();
      refresh();
    });
    QObject::connect(&view, &MainWindow::AxisRequestsPending, page,
                     [=](application::Axis changed, bool pending, bool stopping) {
      if (changed != axis) return;
      availability->pending = pending;
      availability->stopping = stopping;
      refresh();
    });
    QObject::connect(apply, &QPushButton::clicked, page, [=, &view] {
      if (!apply->isEnabled()) return;
      bool ok = false;
      const double value = QLocale::c().toDouble(input->text(), &ok);
      if (!ok || !input->hasAcceptableInput() || !std::isfinite(value) || value <= 0) {
        view.ShowError({axis, "Configure acceleration", "Enter a positive acceleration in mm/s²."});
        return;
      }
      application::MotorSettings settings;
      settings.move.acceleration_mm_per_second_squared = value;
      settings.jog.acceleration_mm_per_second_squared = value;
      feedback->clear();
      emit view.ConfigureAxisRequested(axis, settings);
    });
    QObject::connect(&view, &MainWindow::AxisSettingsApplied, page,
                     [=](application::Axis changed, application::MotorSettings settings) {
      if (changed != axis || !settings.move.acceleration_mm_per_second_squared ||
          settings.move.acceleration_mm_per_second_squared != settings.jog.acceleration_mm_per_second_squared) return;
      feedback->setText(QObject::tr("Applied: %1 mm/s²")
          .arg(*settings.move.acceleration_mm_per_second_squared, 0, 'g', 8));
    });
  }
  acceleration_group->setEnabled(false);
  QObject::connect(&view, &MainWindow::ManualControlsEnabled, acceleration_group, &QWidget::setEnabled);
  layout->addWidget(acceleration_group, 1, 0);
  return page;
}
}  // namespace ui
