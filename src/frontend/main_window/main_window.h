#ifndef FRONTEND_MAIN_WINDOW_H
#define FRONTEND_MAIN_WINDOW_H

#include <QMainWindow>

namespace ui {

class MainWindow : public QMainWindow {
 public:
  explicit MainWindow(QWidget* parent = nullptr);
};

}  // namespace ui

#endif  // FRONTEND_MAIN_WINDOW_H
