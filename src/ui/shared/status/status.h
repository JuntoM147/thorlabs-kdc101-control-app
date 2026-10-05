#ifndef FRONTEND_STATUS_H_
#define FRONTEND_STATUS_H_

#include <QWidget>

#include "main_window/main_window.h"

namespace ui {
QWidget* CreateStatusSection(MainWindow& view, QWidget* parent);
// Connect once per window; every status section displays the same messages.
void ConnectStatusMessages(MainWindow& view);
}  // namespace ui

#endif  // FRONTEND_STATUS_H_
