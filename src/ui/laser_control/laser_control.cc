#include "laser_control.h"
#include "main_window/display_text.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QPainter>
#include <memory>

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSizePolicy>

namespace ui {
namespace {

// Keep native checkbox keyboard/accessibility behavior, but draw a compact switch.
class OutputSwitch : public QCheckBox {
 public:
  explicit OutputSwitch(QWidget* parent) : QCheckBox(parent) {
    setFixedSize(48, 28);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);
  }

 protected:
  bool hitButton(const QPoint& point) const override { return rect().contains(point); }
  // The device observation confirms changes, not the click itself.
  void nextCheckState() override {}
  void paintEvent(QPaintEvent*) override {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor track = !isEnabled() ? QColor("#e0e5ec")
        : isChecked() ? QColor("#126bf0") : QColor("#8794a8");
    painter.setPen(Qt::NoPen);
    painter.setBrush(track);
    painter.drawRoundedRect(QRectF(2, 3, 44, 22), 11, 11);
    painter.setBrush(isEnabled() ? QColor("#ffffff") : QColor("#f3f5f8"));
    painter.drawEllipse(QRectF(isChecked() ? 26 : 5, 6, 16, 16));
    if (hasFocus()) {
      painter.setBrush(Qt::NoBrush);
      painter.setPen(QPen(QColor("#126bf0"), 1, Qt::DashLine));
      painter.drawRoundedRect(QRectF(0.5, 0.5, 47, 27), 13, 13);
    }
  }
};

}  // namespace


QWidget* CreateLaserControlSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Laser control"), parent);

  section->setObjectName(QStringLiteral("laser_control"));
  auto* outer_layout = new QVBoxLayout(section);
  auto* manual = new QWidget(section);
  manual->setObjectName(QStringLiteral("laserManual"));
  manual->setStyleSheet("QWidget#laserManual { background: transparent; }");
  auto* layout = new QVBoxLayout(manual);
  layout->setContentsMargins(0, 0, 0, 0);
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

  auto* output_row = new QHBoxLayout();
  auto* output_label = new QLabel(QObject::tr("Laser output"), section);
  auto* status = new OutputSwitch(section);
  status->setObjectName(QStringLiteral("laserOutputSwitch"));
  status->setAccessibleName(QObject::tr("Laser output status"));
  status->setEnabled(false);
  status->setToolTip(QObject::tr("Connect the laser to control its output."));
  output_label->setBuddy(status);
  output_row->addWidget(output_label);
  output_row->addStretch();
  output_row->addWidget(status);
  layout->addLayout(output_row);

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
  QObject::connect(status, &QCheckBox::clicked, section, [&view, status] {
    emit view.SetLaserOutputRequested(status->property("outputKnown").toBool() && !status->property("outputEnabled").toBool());
  });
  struct Availability {
    application::LaserState state{};
    bool pending = false;
  };
  const auto availability = std::make_shared<Availability>();
  const auto refresh = [=] {
    const auto& state = availability->state;
    const bool connected = state.connection == application::ConnectionState::kConnected;
    connect_button->setProperty("connected", connected);
    connect_button->setText(connected ? QObject::tr("Disconnect") : QObject::tr("Connect"));
    connect_button->setEnabled(!availability->pending && state.connection != application::ConnectionState::kConnecting);
    channel->setEnabled(!availability->pending && !connected && state.connection != application::ConnectionState::kConnecting);
    status->setEnabled(connected && !availability->pending);
    status->setProperty("outputEnabled", state.output_enabled.value_or(false));
    status->setProperty("outputKnown", state.output_enabled.has_value());
    if (!availability->pending) status->setChecked(connected && state.output_enabled.value_or(false));
    status->setToolTip(!connected ? QObject::tr("Connect the laser to control its output.")
        : !state.output_enabled ? QObject::tr("Output state unavailable. Click to request output off.")
        : *state.output_enabled ? QObject::tr("Output on. Click to turn off. Last confirmed command, not measured emission.")
                                : QObject::tr("Output off. Click to turn on. Last confirmed command, not measured emission."));
    connection_status->setText(ConnectionText(state.connection));
    indicator->setStyleSheet(connected ? "background: #00a34a; border-radius: 6px;"
                                      : "background: #ed1735; border-radius: 6px;");
  };
  QObject::connect(&view, &MainWindow::LaserStateUpdated, section, [=](application::LaserState state) {
    availability->state = state;
    refresh();
  });
  QObject::connect(&view, &MainWindow::LaserRequestPending, section, [=](bool pending) {
    availability->pending = pending;
    refresh();
  });
  manual->setEnabled(false);
  QObject::connect(&view, &MainWindow::ManualControlsEnabled, manual, &QWidget::setEnabled);
  outer_layout->addStretch();

  return section;
}

}  // namespace ui
