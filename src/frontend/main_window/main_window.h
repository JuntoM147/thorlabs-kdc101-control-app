#ifndef FRONTEND_MAIN_WINDOW_H
#define FRONTEND_MAIN_WINDOW_H

#include <QMainWindow>
#include <QImage>

#include "../../application/application_types/application_types.h"

namespace ui {

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  void SetBackendAvailable(bool available);
  void SetScanImage(const QImage& image);
  void SetPixelSize(double micrometres);
  void SetExposureTime(int milliseconds);
  bool CanStartScan() const;
  bool HasBackend() const { return backend_available_; }
  application::ScanPhase ScanPhase() const { return scan_state_.phase; }

 public slots:
  void SetControlsLocked(bool locked);
  void UpdateScanState(application::ScanState state);
  void ShowError(application::OperationError error);
  void RequestScan();

 signals:
  // User intent. Application decides whether requests can be serviced.
  void ConnectMotorRequested(application::MotorConnection connection);
  void DisconnectMotorRequested(application::Axis axis);
  void HomeAxisRequested(application::Axis axis);
  void MoveAxisRequested(application::Axis axis, double position_mm, application::MotionSettings settings);
  void JogAxisRequested(application::Axis axis, application::Direction direction, double step_mm, application::MotionSettings settings);
  void DriveAxisRequested(application::Axis axis, application::Direction direction, application::MotionSettings settings);
  void StopAxisRequested(application::Axis axis, application::StopMode mode);
  void ConnectLaserRequested(application::LaserConnection connection);
  void DisconnectLaserRequested();
  void SetLaserOutputRequested(bool enabled);
  void StartScanRequested(application::ScanConfiguration configuration);
  void PauseScanRequested();
  void ResumeScanRequested();
  void CancelScanRequested();

  // Broadcast application observations to the display sections.
  void AxisStateUpdated(application::AxisState state);
  void LaserStateUpdated(application::LaserState state);
  void ScanDisplayChanged(application::ScanState state);
  void ManualControlsEnabled(bool enabled);
  void ScanInputsEnabled(bool enabled);
  void ScanAvailabilityChanged();

 private:
  bool backend_available_ = false;
  bool controls_locked_ = false;
  bool has_pattern_ = false;
  application::ScanConfiguration scan_configuration_;
  application::ScanState scan_state_;
};

}  // namespace ui

#endif  // FRONTEND_MAIN_WINDOW_H
