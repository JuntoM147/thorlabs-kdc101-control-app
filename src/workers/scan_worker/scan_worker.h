#ifndef WORKERS_SCAN_WORKER_H_
#define WORKERS_SCAN_WORKER_H_

#include <QElapsedTimer>
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
  void OnMotorDisconnected(workers::Axis axis);
  void OnPositionCaptured(workers::Axis axis, workers::RequestId id,
                          double position_mm);

 signals:
  void PositionRequested(workers::Axis axis, workers::RequestId id);
  void MoveRequested(workers::Axis axis, workers::RequestId id,
                     double position_mm);
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
  bool capturing_origin_ = false;
  double origin_x_mm_ = 0;
  double origin_y_mm_ = 0;
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
  QTimer* progress_timer_ = nullptr;
  QElapsedTimer active_timer_;
  qint64 active_elapsed_ms_ = 0;
};

}  // namespace workers

#endif  // WORKERS_SCAN_WORKER_H_
