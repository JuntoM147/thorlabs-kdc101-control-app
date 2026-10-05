#ifndef FRONTEND_DISPLAY_TEXT_H_
#define FRONTEND_DISPLAY_TEXT_H_

#include <QObject>
#include <QString>

#include "ui_types.h"

namespace ui {
inline QString ConnectionText(ui::ConnectionState state) {
  switch (state) {
    case ui::ConnectionState::kDisconnected:
      return QObject::tr("Disconnected");
    case ui::ConnectionState::kConnecting:
      return QObject::tr("Connecting");
    case ui::ConnectionState::kConnected:
      return QObject::tr("Connected");
    case ui::ConnectionState::kFaulted:
      return QObject::tr("Faulted");
  }
  return QObject::tr("Unknown");
}

inline QString OperationText(ui::OperationState state) {
  switch (state) {
    case ui::OperationState::kIdle:
      return QObject::tr("Idle");
    case ui::OperationState::kHoming:
      return QObject::tr("Homing");
    case ui::OperationState::kMoving:
      return QObject::tr("Moving");
    case ui::OperationState::kJogging:
      return QObject::tr("Jogging");
    case ui::OperationState::kDriving:
      return QObject::tr("Driving");
    case ui::OperationState::kStopping:
      return QObject::tr("Stopping");
  }
  return QObject::tr("Unknown");
}
}  // namespace ui
#endif  // FRONTEND_DISPLAY_TEXT_H_
