#ifndef APPLICATION_MOTOR_WORKER_H_
#define APPLICATION_MOTOR_WORKER_H_

#include <memory>
#include <functional>
#include <optional>
#include <deque>

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>

#include "../application_types/application_types.h"
#include "../../devices/kdc101/kdc101.h"

namespace application {

// Dedicated thread to control the motor hardware
class MotorWorker : public QObject {
  Q_OBJECT

 public:
  explicit MotorWorker(Axis axis, QObject* parent = nullptr,
      std::shared_ptr<const thorlabs::KinesisSimulation> simulation = nullptr);
  ~MotorWorker() override;

 public slots:
  void Connect(int id, MotorConnection connection);
  void Disconnect(int id);
  void ConfigureMotion(int id, MotorSettings settings);
  void Home(int id);
  void MoveAbsolute(int id, double position_mm);
  void MoveRelative(int id, double distance_mm, bool diagnostic = false);
  void Jog(int id, Direction direction);
  void Drive(int id, Direction direction);
  void Stop(int id, StopMode mode);
  void Shutdown();

 signals:
  void StateChanged(application::AxisState state);
  void RequestAccepted(int id);
  void RequestCompleted(int id);
  void RequestFailed(int id, application::OperationError error);
  void RequestCancelled(int id);
  void ShutdownReady(application::Axis axis);

 private slots:
  void PollDevice();

 private:
  void BeginMotion(int id, OperationState operation,
                   const std::function<thorlabs::DeviceStatus()>& start);
  void FailDevice(OperationError error);
  void TracePosition(const char* phase, qint64 timestamp, double position, bool moving);
  thorlabs::DeviceStatus ApplyMotionSettings(const MotorSettings& settings);
  OperationState operation_ = OperationState::kIdle;  // Current command, not feedback.
  const Axis axis_;
  std::shared_ptr<const thorlabs::KinesisSimulation> simulation_;
  std::unique_ptr<thorlabs::KDC101> motor_; // owns the RAII motor wrapper
  QTimer* poll_timer_ = nullptr;
  std::optional<int> active_request_;  // Motion request awaiting completion
  std::optional<int> stop_request_;  // Stop request awaiting confirmation of stopped motion
  struct RelativeMoveCheck {
    double start_mm;
    double distance_mm;
    double tolerance_mm;
    QElapsedTimer completed_since;
    bool diagnostic = false;
    int id = 0;
  };
  std::optional<RelativeMoveCheck> relative_move_check_;
  struct PositionSample { qint64 timestamp; double position; bool moving; };
  std::deque<PositionSample> recent_positions_;
  QString trace_session_;
  std::string serial_number_;
  bool shutting_down_ = false;  // Reject new commands while hardware cleanup runs
};

}  // namespace application

#endif  // APPLICATION_MOTOR_WORKER_H_
