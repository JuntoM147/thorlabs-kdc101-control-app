#ifndef FRONTEND_APPLICATION_BINDING_H_
#define FRONTEND_APPLICATION_BINDING_H_

#include <QObject>
#include "main_window/main_window.h"

namespace ui {

// Set up Qt connections between the window and the Application object
template <typename Application>
void BindApplication(MainWindow& view, Application& application) {
  QObject::connect(&view, &MainWindow::ConnectMotorRequested, &application, &Application::ConnectMotor);
  QObject::connect(&view, &MainWindow::DisconnectMotorRequested, &application, &Application::DisconnectMotor);
  QObject::connect(&view, &MainWindow::ConfigureAxisRequested, &application, &Application::ConfigureAxis);
  QObject::connect(&view, &MainWindow::HomeAxisRequested, &application, &Application::HomeAxis);
  QObject::connect(&view, &MainWindow::MoveAxisRequested, &application, &Application::MoveAxis);
  QObject::connect(&view, &MainWindow::JogAxisRequested, &application, &Application::JogAxis);
  QObject::connect(&view, &MainWindow::DriveAxisRequested, &application, &Application::DriveAxis);
  QObject::connect(&view, &MainWindow::StopAxisRequested, &application, &Application::StopAxis);
  QObject::connect(&view, &MainWindow::ConnectLaserRequested, &application, &Application::ConnectLaser);
  QObject::connect(&view, &MainWindow::DisconnectLaserRequested, &application, &Application::DisconnectLaser);
  QObject::connect(&view, &MainWindow::SetLaserOutputRequested, &application, &Application::SetLaserOutput);
  QObject::connect(&view, &MainWindow::StartScanRequested, &application, &Application::StartScan);
  QObject::connect(&view, &MainWindow::PauseScanRequested, &application, &Application::PauseScan);
  QObject::connect(&view, &MainWindow::ResumeScanRequested, &application, &Application::ResumeScan);
  QObject::connect(&view, &MainWindow::CancelScanRequested, &application, &Application::CancelScan);

  QObject::connect(&application, &Application::AxisSettingsApplied, &view, &MainWindow::AxisSettingsApplied);
  QObject::connect(&application, &Application::AxisStateUpdated, &view, &MainWindow::AxisStateUpdated);
  QObject::connect(&application, &Application::AxisRequestsPending, &view, &MainWindow::AxisRequestsPending);
  QObject::connect(&application, &Application::LaserStateUpdated, &view, &MainWindow::LaserStateUpdated);
  QObject::connect(&application, &Application::ManualRequestsPending, &view, &MainWindow::SetManualRequestsPending);
  QObject::connect(&application, &Application::ControlsLocked, &view, &MainWindow::SetControlsLocked);
  QObject::connect(&application, &Application::ScanStateUpdated, &view, &MainWindow::UpdateScanState);
  QObject::connect(&application, &Application::RequestFailed, &view, &MainWindow::ShowError);
  QObject::connect(&application, &QObject::destroyed, &view, [&view] { view.SetBackendAvailable(false); });
  view.SetBackendAvailable(true);
  view.SetManualRequestsPending(application.HasPendingManualRequests());
  for (int i = 0; i < 3; ++i) {
    const auto axis = static_cast<application::Axis>(i);
    emit view.AxisRequestsPending(axis, application.HasPendingAxisRequest(axis), application.HasPendingAxisStop(axis));
  }
}

}  // namespace ui
#endif  // FRONTEND_APPLICATION_BINDING_H_