#ifndef WORKERS_SCAN_WORKER_H_
#define WORKERS_SCAN_WORKER_H_

#include <QObject>
#include <QTimer>
#include <optional>

#include "workers/worker_types.h"

namespace workers {

class ScanWorker final : public QObject {
  Q_OBJECT

 public:
  explicit ScanWorker(QObject* parent = nullptr);
  ~ScanWorker() override;

 public slots:
  void Start(workers::ScanJob job);
  void Pause();
  void Resume();
  void Cancel();
  void Shutdown();

  void OnMotorFinished(workers::Axis axis, workers::RequestId id,
                       errors::Error result);
  void OnLaserFinished(workers::RequestId id, errors::Error result);

 signals:
  void MoveRequested(workers::Axis axis, workers::RequestId id,
                     double distance_mm);
  void StopRequested(workers::Axis axis, workers::RequestId id,
                     thorlabs::StopMode mode);
  void LaserOutputRequested(workers::RequestId id, bool enabled);
  void ProgressChanged(workers::ScanProgress progress);
  // Completed normally, cancelled, or failed. Report cleanup failures as well.
  void Finished(errors::Error result);
  void ShutdownFinished();

 private:
  void ExecuteNextInstruction();
  void CompleteInstruction();
  void BeginCleanup(errors::Error result);
  void OnTimeout();
  void OnExposureFinished();
  void PublishProgress();
  RequestId NextRequestId();
  void EnsureTimers();
  void CheckPendingRequests();
  void FinishCleanup();
  void RequestLaser(bool enabled);

  ScanJob job_;
  ScanProgress progress_;
  bool pause_requested_ = false;
  bool shutdown_requested_ = false;
  bool program_laser_on_ = false;
  bool restoring_laser_ = false;
  bool pause_laser_off_ = false;
  bool cleanup_failed_ = false;
  RequestId next_request_id_ = 1;  // Never reset between scans.
  std::optional<RequestId> pending_x_;
  std::optional<RequestId> pending_y_;
  std::optional<RequestId> pending_laser_;
  std::optional<errors::Error> final_result_;

  QTimer* operation_timer_ = nullptr;
  QTimer* exposure_timer_ = nullptr;
};

}  // namespace workers

#endif  // WORKERS_SCAN_WORKER_H_
