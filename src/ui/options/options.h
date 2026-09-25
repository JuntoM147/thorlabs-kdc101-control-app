#ifndef UI_OPTIONS_H_
#define UI_OPTIONS_H_

#include <QWidget>

namespace ui {
class MainWindow;
QWidget* CreateOptionsSection(MainWindow& view, QWidget* parent);
}  // namespace ui

#endif  // UI_OPTIONS_H_
