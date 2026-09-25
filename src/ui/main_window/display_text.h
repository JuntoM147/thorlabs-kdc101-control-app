#ifndef FRONTEND_DISPLAY_TEXT_H_
#define FRONTEND_DISPLAY_TEXT_H_

#include <QObject>
#include <QString>
#include "../../application/application_types/application_types.h"

namespace ui {
inline QString ConnectionText(application::ConnectionState state) {
  switch (state) {
    case application::ConnectionState::kDisconnected: return QObject::tr("Disconnected");
    case application::ConnectionState::kConnecting: return QObject::tr("Connecting");
    case application::ConnectionState::kConnected: return QObject::tr("Connected");
    case application::ConnectionState::kFaulted: return QObject::tr("Faulted");
  }
  return QObject::tr("Unknown");
}

inline QString OperationText(application::OperationState state) {
  switch (state) {
    case application::OperationState::kIdle: return QObject::tr("Idle");
    case application::OperationState::kHoming: return QObject::tr("Homing");
    case application::OperationState::kMoving: return QObject::tr("Moving");
    case application::OperationState::kJogging: return QObject::tr("Jogging");
    case application::OperationState::kDriving: return QObject::tr("Driving");
    case application::OperationState::kStopping: return QObject::tr("Stopping");
  }
  return QObject::tr("Unknown");
}
}  // namespace ui
#endif  // FRONTEND_DISPLAY_TEXT_H_
