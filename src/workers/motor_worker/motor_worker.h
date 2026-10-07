#ifndef WORKERS_MOTOR_WORKER_H_
#define WORKERS_MOTOR_WORKER_H_

#include <QObject>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>
#include <functional>
#include <memory>
#include <optional>

#include "workers/worker_types.h"

namespace workers {

class MotorWorker final : public QObject {
  Q_OBJECT

 public:
  explicit MotorWorker(QObject* parent = nullptr);
  ~MotorWorker() override;

 public slots:
  void ConnectDevice(workers::RequestId id, QString serial_number,
                     bool simulation = false);
  void DisconnectDevice(workers::RequestId id);
  void Configure(workers::RequestId id, workers::MotorSettings settings);
  void Home(workers::RequestId id);
  void CapturePosition(workers::RequestId id);
  void MoveAbsolute(workers::RequestId id, double position_mm);
  void MoveRelative(workers::RequestId id, double distance_mm);
  void Jog(workers::RequestId id, thorlabs::Direction direction);
  void Drive(workers::RequestId id, thorlabs::Direction direction);
  void Stop(workers::RequestId id,
            thorlabs::StopMode mode = thorlabs::StopMode::kProfiled);
  void Shutdown();

 signals:
  void PositionCaptured(workers::RequestId id, double position_mm);
  void RequestFinished(workers::RequestId id, errors::Error result);
  void StateChanged(workers::MotorState state);
  void SettingsChanged(workers::MotorSettings settings);
  void PollingFailed(errors::Error error);
  void ShutdownFinished();

 private:
  struct PendingMotion {
    RequestId id;
    thorlabs::MotorEvent completion_event;
    bool confirm_idle = false;
    bool completion_received = false;
    int idle_polls = 0;
    std::optional<double> target_mm;
  };

  void PollDevice();
  void PublishState();
  errors::Error PublishSettings();
  void FinishMotion(errors::Error result);
  void FinishStop(errors::Error result);
  bool CheckReady(RequestId id);
  void StartMotion(RequestId id, thorlabs::MotorEvent completion,
                   std::function<errors::Error()> command,
                   bool continuous = false);
  void OnTimeout();
  void CheckAbsolutePosition();

  std::unique_ptr<thorlabs::KDC101> motor_;
  std::shared_ptr<const thorlabs::KinesisSimulation> simulation_;
  QTimer* poll_timer_ =
      nullptr;  // QObject child, created in the worker thread.
  std::optional<PendingMotion> pending_motion_;
  std::optional<RequestId> pending_stop_;
  QTimer* motion_timer_ = nullptr;
  QElapsedTimer motion_elapsed_;
  bool shutting_down_ = false;
  int stop_idle_polls_ = 0;
};

}  // namespace workers

#endif  // WORKERS_MOTOR_WORKER_H_
