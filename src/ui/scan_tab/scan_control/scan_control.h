#ifndef FRONTEND_SCAN_CONTROL_H
#define FRONTEND_SCAN_CONTROL_H

#include <QWidget>

#include "main_window/main_window.h"

namespace ui {

QWidget* CreateScanControlSection(MainWindow& view, QWidget* parent);

}  // namespace ui

#endif  // FRONTEND_SCAN_CONTROL_H
