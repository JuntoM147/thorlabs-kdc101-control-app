#ifndef FRONTEND_MOTOR_CONTROL_H
#define FRONTEND_MOTOR_CONTROL_H

#include <QWidget>

#include "main_window/main_window.h"

namespace ui {

QWidget* CreateMotorControlSection(MainWindow& view, QWidget* parent);

}  // namespace ui

#endif  // FRONTEND_MOTOR_CONTROL_H
