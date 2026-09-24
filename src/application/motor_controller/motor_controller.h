#ifndef APPLICATION_MOTOR_CONTROLLER_H_
#define APPLICATION_MOTOR_CONTROLLER_H_

#include <optional>

#include <QObject>
#include <QPointer>
#include <QThread>

#include "../application_types/application_types.h"
#include "../motor_worker/motor_worker.h"

namespace application {

// Manages a worker thread to control the motor hardware
class MotorController : public QObject {
  Q_OBJECT

 public:
  explicit MotorController(Axis axis, QObject* parent = nullptr);
  ~MotorController() override;

 public slots:
  void Connect(int id, MotorConnection connection);
  void Disconnect(int id);
  void Home(int id);
  void MoveAbsolute(int id, double position_mm, MotionSettings settings);
  void MoveRelative(int id, double distance_mm, MotionSettings settings);
  void Jog(int id, Direction direction, double step_mm, MotionSettings settings);
  void Drive(int id, Direction direction, MotionSettings settings);
  void Stop(int id, StopMode mode);

  // ID-free scan API. Application reserves access; reject manual commands while
  // scan work is pending. At most one scan operation may be outstanding.
  void MoveRelativeForScan(double distance_mm, MotionSettings settings);
  void StopForScan(StopMode mode);
  // Stop active motion and drain both original/stop replies before notification.
  void CancelScanOperation();

 signals:
  void StateChanged(application::AxisState state);
  void RequestAccepted(int id);
  void RequestCompleted(int id);
  void RequestFailed(int id, application::OperationError error);
  void RequestCancelled(int id);

  // Only private scan IDs produce these signals, once the operation has settled.
  // Terminal callbacks run on the GUI thread. Retire IDs before emitting.
  void ScanOperationCompleted(application::Axis axis);
  void ScanOperationFailed(application::Axis axis, application::OperationError error);
  void ScanOperationCancelled(application::Axis axis);

 private slots:
  void OnWorkerCompleted(int id);
  void OnWorkerFailed(int id, OperationError error);
  void OnWorkerCancelled(int id);

 private:
  // Positive IDs belong to manual callers. Allocate negative scan IDs without
  // reuse; at INT_MIN mark exhaustion instead of overflowing. Zero is reserved.
  // Never forward retired negative IDs as completions. Preserve positive-ID
  // replies for Application; unsolicited device faults must still be reported.
  [[nodiscard]] int NextScanRequestId();
  void EnsureWorkerStarted();
  void StopWorkerAndWait();

  const Axis axis_;
  int next_scan_request_id_ = -1;
  std::optional<int> scan_request_;
  std::optional<int> scan_stop_request_;  // Cancellation awaits both replies.
  std::optional<OperationError> scan_error_;  // Retain failure while settling stop.
  QThread thread_;
  QPointer<MotorWorker> worker_;
};

}  // namespace application

#endif  // APPLICATION_MOTOR_CONTROLLER_H_
