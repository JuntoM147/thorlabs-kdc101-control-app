#include "motor_controller.h"

#include <climits>
#include <utility>

namespace application {
MotorController::MotorController(Axis axis, QObject* parent,
    std::shared_ptr<const thorlabs::KinesisSimulation> simulation)
    : QObject(parent), axis_(axis), simulation_(std::move(simulation)) {}
MotorController::~MotorController() { StopWorkerAndWait(); }
int MotorController::NextScanRequestId() {
  const int id = next_scan_request_id_;
  if (id != 0) next_scan_request_id_ = id == INT_MIN ? 0 : id - 1;
  return id;
}
void MotorController::EnsureWorkerStarted() {
  if (worker_) return;
  worker_ = new MotorWorker(axis_, nullptr, simulation_);
  worker_->moveToThread(&thread_);
  connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
  connect(worker_, &MotorWorker::StateChanged, this, &MotorController::StateChanged);
  connect(worker_, &MotorWorker::PositionWarning, this, &MotorController::PositionWarning);
  connect(worker_, &MotorWorker::RequestAccepted, this, [this](int id) {
    if (id > 0) emit RequestAccepted(id);
  });
  connect(worker_, &MotorWorker::RequestCompleted, this, &MotorController::OnWorkerCompleted);
  connect(worker_, &MotorWorker::RequestFailed, this, &MotorController::OnWorkerFailed);
  connect(worker_, &MotorWorker::RequestCancelled, this, &MotorController::OnWorkerCancelled);
  thread_.start();
}
void MotorController::StopWorkerAndWait() {
  if (!thread_.isRunning()) return;
  // Shutdown executes on the device thread and does not need GUI callbacks.
  QMetaObject::invokeMethod(worker_, &MotorWorker::Shutdown, Qt::BlockingQueuedConnection);
  thread_.quit();
  thread_.wait();
  worker_ = nullptr;
}
void MotorController::Dispatch(int id, const std::function<void(MotorWorker&)>& command) {
  if (!worker_ || disconnect_request_ || (id > 0 && (scan_request_ || scan_stop_request_))) {
    OnWorkerFailed(id, {axis_, "Motor command", "Controller is disconnected or reserved."});
    return;
  }
  auto* worker = worker_.data();
  QMetaObject::invokeMethod(worker, [worker, command] { command(*worker); }, Qt::QueuedConnection);
}
void MotorController::Connect(int id, MotorConnection connection) {
  if (id <= 0 || disconnect_request_ || scan_request_ || scan_stop_request_) {
    emit RequestFailed(id, {axis_, "Connect", "Controller is reserved."}); return;
  }
  EnsureWorkerStarted();
  Dispatch(id, [=](MotorWorker& worker) { worker.Connect(id, connection); });
}
void MotorController::Disconnect(int id) {
  if (!worker_) { emit RequestCompleted(id); return; }
  if (disconnect_request_ || scan_request_ || scan_stop_request_) {
    emit RequestFailed(id, {axis_, "Disconnect", "Controller is reserved."}); return;
  }
  disconnect_request_ = id;
  auto* worker = worker_.data();
  QMetaObject::invokeMethod(worker, [worker, id] { worker->Disconnect(id); }, Qt::QueuedConnection);
}
void MotorController::ConfigureMotion(int id, MotorSettings settings) {
  Dispatch(id, [=](MotorWorker& worker) { worker.ConfigureMotion(id, settings); });
}
void MotorController::Home(int id) {
  Dispatch(id, [=](MotorWorker& worker) { worker.Home(id); });
}
void MotorController::MoveAbsolute(int id, double position) {
  Dispatch(id, [=](MotorWorker& worker) { worker.MoveAbsolute(id, position); });
}
void MotorController::MoveRelative(int id, double distance) {
  Dispatch(id, [=](MotorWorker& worker) { worker.MoveRelative(id, distance); });
}
void MotorController::Jog(int id, Direction direction) {
  Dispatch(id, [=](MotorWorker& worker) { worker.Jog(id, direction); });
}
void MotorController::Drive(int id, Direction direction) {
  Dispatch(id, [=](MotorWorker& worker) { worker.Drive(id, direction); });
}
void MotorController::Stop(int id, StopMode mode) {
  Dispatch(id, [=](MotorWorker& worker) { worker.Stop(id, mode); });
}
void MotorController::ConfigureForScan(MotionSettings settings) {
  if (scan_request_ || scan_stop_request_) {
    emit ScanOperationFailed(axis_, {axis_, "Configure scan motion", "An operation is already pending."}); return;
  }
  const int id = NextScanRequestId();
  if (!id) { emit ScanOperationFailed(axis_, {axis_, "Configure scan motion", "Request IDs exhausted."}); return; }
  scan_request_ = id;
  Dispatch(id, [=](MotorWorker& worker) {
    MotorSettings configuration;
    configuration.move = settings;
    worker.ConfigureMotion(id, configuration);
  });
}
void MotorController::MoveRelativeForScan(double distance) {
  if (scan_request_ || scan_stop_request_) {
    emit ScanOperationFailed(axis_, {axis_, "Scan move", "An operation is already pending."}); return;
  }
  const int id = NextScanRequestId();
  if (!id) { emit ScanOperationFailed(axis_, {axis_, "Scan move", "Request IDs exhausted."}); return; }
  scan_request_ = id;
  Dispatch(id, [=](MotorWorker& worker) { worker.MoveRelative(id, distance); });
}
void MotorController::StopForScan(StopMode mode) {
  if (scan_request_ || scan_stop_request_) {
    emit ScanOperationFailed(axis_, {axis_, "Scan stop", "An operation is already pending."}); return;
  }
  const int id = NextScanRequestId();
  if (!id) { emit ScanOperationFailed(axis_, {axis_, "Scan stop", "Request IDs exhausted."}); return; }
  scan_request_ = id;
  Dispatch(id, [=](MotorWorker& worker) { worker.Stop(id, mode); });
}
void MotorController::CancelScanOperation() {
  if (!scan_request_ || scan_cancelled_) return;
  scan_cancelled_ = true;
  const int id = NextScanRequestId();
  if (!id) {
    scan_error_ = OperationError{axis_, "Cancel scan", "Request IDs exhausted."};
    return;  // Retain the original request until its terminal reply.
  }
  scan_stop_request_ = id;
  Dispatch(id, [=](MotorWorker& worker) { worker.Stop(id, StopMode::kImmediate); });
}
void MotorController::FinishScanRequest() {
  if (scan_request_ || scan_stop_request_) return;
  const auto error = std::exchange(scan_error_, {});
  const bool cancelled = std::exchange(scan_cancelled_, false);
  if (error) emit ScanOperationFailed(axis_, *error);
  else if (cancelled) emit ScanOperationCancelled(axis_);
  else emit ScanOperationCompleted(axis_);
}
void MotorController::OnWorkerCompleted(int id) {
  if (id > 0) {
    if (disconnect_request_ == id) {
      StopWorkerAndWait();
      disconnect_request_.reset();
    }
    emit RequestCompleted(id);
  } else if (scan_request_ == id || scan_stop_request_ == id) {
    if (scan_request_ == id) scan_request_.reset();
    if (scan_stop_request_ == id) scan_stop_request_.reset();
    FinishScanRequest();
  }
}
void MotorController::OnWorkerFailed(int id, OperationError error) {
  if (id >= 0) {
    if (disconnect_request_ == id) disconnect_request_.reset();
    emit RequestFailed(id, error);
  } else if (scan_request_ == id || scan_stop_request_ == id) {
    if (!scan_error_) scan_error_ = error;
    if (scan_request_ == id) scan_request_.reset();
    if (scan_stop_request_ == id) scan_stop_request_.reset();
    FinishScanRequest();
  }
}
void MotorController::OnWorkerCancelled(int id) {
  if (id > 0) emit RequestCancelled(id);
  else if (scan_request_ == id || scan_stop_request_ == id) {
    scan_cancelled_ = true;
    if (scan_request_ == id) scan_request_.reset();
    if (scan_stop_request_ == id) scan_stop_request_.reset();
    FinishScanRequest();
  }
}
}  // namespace application
