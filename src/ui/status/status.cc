#include "status.h"
#include "main_window/display_text.h"

#include <array>
#include <QGroupBox>
#include <QLabel>
#include <QStringList>
#include <QVBoxLayout>

namespace ui {

QWidget* CreateStatusSection(MainWindow& view, QWidget* parent) {
  auto* section = new QGroupBox(QObject::tr("Status"), parent);
  section->setObjectName(QStringLiteral("status"));
  auto* layout = new QVBoxLayout(section);
  auto* message = new QLabel(section);
  message->setAccessibleName(QObject::tr("Latest status message"));
  message->setWordWrap(true);
  message->setTextFormat(Qt::PlainText);
  message->setTextInteractionFlags(Qt::TextSelectableByMouse);
  message->setMinimumHeight(48);
  layout->addWidget(message);
  QObject::connect(&view, &MainWindow::StatusMessageChanged, section,
                   [message](const QString& text, bool error) {
    message->setText(text);
    message->setProperty("error", error);
    message->setStyleSheet(error ? "color: #b42338;" : "color: #00843b;");
  });

  // Only state transitions produce messages; position polling must not erase errors.
  QObject::connect(&view, &MainWindow::AxisStateUpdated, section,
      [&view, previous = std::array<application::AxisState, 3>{}]
      (application::AxisState state) mutable {
    const auto index = static_cast<std::size_t>(state.axis);
    if (index >= previous.size()) return;
    const auto old = previous[index];
    previous[index] = state;
    const auto prefix = QObject::tr("%1 axis: ").arg(QStringList{"X", "Y", "Z"}.at(index));
    if (state.connection != old.connection) {
      view.ShowMessage(prefix + ConnectionText(state.connection),
                       state.connection == application::ConnectionState::kFaulted);
    } else if (state.connection == application::ConnectionState::kConnected &&
               state.operation != old.operation) {
      view.ShowMessage(prefix + OperationText(state.operation));
    }
  });
  QObject::connect(&view, &MainWindow::LaserStateUpdated, section,
      [&view, previous = application::LaserState{}](application::LaserState state) mutable {
    const auto old = previous;
    previous = state;
    if (state.connection != old.connection) {
      view.ShowMessage(QObject::tr("Laser: ") + ConnectionText(state.connection),
                       state.connection == application::ConnectionState::kFaulted);
    } else if (state.connection == application::ConnectionState::kConnected &&
               state.output_enabled != old.output_enabled && state.output_enabled) {
      view.ShowMessage(*state.output_enabled ? QObject::tr("Laser output enabled.")
                                            : QObject::tr("Laser output disabled."));
    }
  });
  QObject::connect(&view, &MainWindow::ScanDisplayChanged, section,
      [&view, previous = application::ScanPhase::kIdle](application::ScanState state) mutable {
    if (state.phase == previous) return;
    previous = state.phase;
    using application::ScanPhase;
    QString text;
    switch (state.phase) {
      case ScanPhase::kIdle: text = QObject::tr("Scan idle."); break;
      case ScanPhase::kWaiting: text = QObject::tr("Scan requested."); break;
      case ScanPhase::kPreparing: text = QObject::tr("Preparing scan."); break;
      case ScanPhase::kRunning: text = QObject::tr("Scan running."); break;
      case ScanPhase::kPauseRequested: text = QObject::tr("Scan pause requested."); break;
      case ScanPhase::kPausing: text = QObject::tr("Pausing scan."); break;
      case ScanPhase::kPaused: text = QObject::tr("Scan paused."); break;
      case ScanPhase::kResuming: text = QObject::tr("Resuming scan."); break;
      case ScanPhase::kStopping: text = QObject::tr("Stopping scan."); break;
      case ScanPhase::kFailed: text = QObject::tr("Scan failed."); break;
    }
    view.ShowMessage(text, state.phase == ScanPhase::kFailed);
  });
  return section;
}

}  // namespace ui
