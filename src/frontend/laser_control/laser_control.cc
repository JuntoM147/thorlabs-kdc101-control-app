#include "laser_control.h"
#include "main_window/display_text.h"
#include <QLineEdit>
#include <QSpinBox>
#include <QStyle>

#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSizePolicy>

namespace ui {

QWidget* CreateLaserControlSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Laser control"), parent);

  auto* outer_layout = new QVBoxLayout(section);
  auto* manual = new QWidget(section);
  auto* layout = new QVBoxLayout(manual);
  outer_layout->addWidget(manual);
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
  connect_button->setProperty("connected", false);
  layout->addWidget(connect_button);

  auto* status = new QPushButton(QObject::tr("Output unknown"), section);
  status->setObjectName(QStringLiteral("laserStatus"));
  status->setAccessibleName(QObject::tr("Laser output status"));
  status->setEnabled(false);
  status->setMinimumWidth(120);
  status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  status->setToolTip(QObject::tr("Last successful output command; not measured emission."));
  layout->addWidget(status);


  auto* channel = new QLineEdit(QStringLiteral("Dev1/port0/line0"), section);
  channel->setAccessibleName(QObject::tr("Laser digital output channel"));
  layout->insertWidget(1, channel);
  QObject::connect(connect_button, &QPushButton::clicked, section, [&view, connect_button, channel] {
    if (connect_button->property("connected").toBool()) {
      emit view.DisconnectLaserRequested();
    } else if (channel->text().trimmed().isEmpty()) {
      view.ShowError({{}, "Connect laser", "Enter a digital output channel."});
    } else {
      emit view.ConnectLaserRequested({channel->text().trimmed().toStdString()});
    }
  });
  QObject::connect(status, &QPushButton::clicked, section, [&view, status] {
    emit view.SetLaserOutputRequested(status->property("outputKnown").toBool() && !status->property("outputEnabled").toBool());
  });
  QObject::connect(&view, &MainWindow::LaserStateUpdated, section, [=](application::LaserState state) {
    const bool connected = state.connection == application::ConnectionState::kConnected;
    connect_button->setProperty("connected", connected);
    connect_button->setText(connected ? QObject::tr("Disconnect") : QObject::tr("Connect"));
    connect_button->setEnabled(state.connection != application::ConnectionState::kConnecting);
    channel->setEnabled(!connected && state.connection != application::ConnectionState::kConnecting);
    status->setEnabled(connected);
    status->setProperty("outputEnabled", state.output_enabled.value_or(false));
    status->setProperty("outputKnown", state.output_enabled.has_value());
    status->setText(!state.output_enabled ? QObject::tr("Output unknown")
                      : *state.output_enabled ? QObject::tr("Laser ON") : QObject::tr("Laser OFF"));
    
    status->setProperty("outputOn", state.output_enabled.value_or(false));
    status->style()->unpolish(status);
    status->style()->polish(status);
    connection_status->setText(ConnectionText(state.connection));
    indicator->setStyleSheet(connected ? "background: #00a34a; border-radius: 6px;"
                                      : "background: #ed1735; border-radius: 6px;");
  });
  manual->setEnabled(false);
  QObject::connect(&view, &MainWindow::ManualControlsEnabled, manual, &QWidget::setEnabled);
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
  outer_layout->addLayout(pixel_row);

  auto* exposure = new QSpinBox(section);
  exposure->setRange(0, 3600000);
  exposure->setSuffix(QObject::tr(" ms"));
  exposure->setAccessibleName(QObject::tr("Exposure time in milliseconds"));
  outer_layout->addWidget(new QLabel(QObject::tr("Exposure time"), section));
  outer_layout->addWidget(exposure);
  outer_layout->addStretch();

  view.SetPixelSize(pixel_size->value());
  QObject::connect(pixel_size, &QDoubleSpinBox::valueChanged, &view, &MainWindow::SetPixelSize);
  QObject::connect(exposure, &QSpinBox::valueChanged, &view, &MainWindow::SetExposureTime);
  QObject::connect(&view, &MainWindow::ScanInputsEnabled, pixel_size, &QWidget::setEnabled);
  QObject::connect(&view, &MainWindow::ScanInputsEnabled, exposure, &QWidget::setEnabled);

  return section;
}

}  // namespace ui
