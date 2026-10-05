#ifndef FRONTEND_LASER_CONTROL_H
#define FRONTEND_LASER_CONTROL_H

#include <QWidget>

#include "main_window/main_window.h"

namespace ui {

QWidget* CreateLaserControlSection(MainWindow& view, QWidget* parent);

}  // namespace ui

#endif  // FRONTEND_LASER_CONTROL_H
