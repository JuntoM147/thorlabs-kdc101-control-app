#include "laser_controller.h"

#include <climits>
#include <utility>

namespace application {
LaserController::LaserController(QObject* parent) : QObject(parent) {}
LaserController::~LaserController() { StopWorkerAndWait(); }
int LaserController::NextScanRequestId() {
  const int id = next_scan_request_id_;
  if (id) next_scan_request_id_ = id == INT_MIN ? 0 : id - 1;
  return id;
}
void LaserController::EnsureWorkerStarted() {
  if (worker_) return;
  worker_ = new LaserWorker;
  worker_->moveToThread(&thread_);
  connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
  connect(worker_, &LaserWorker::StateChanged, this, &LaserController::StateChanged);
  connect(worker_, &LaserWorker::RequestCompleted, this, &LaserController::OnWorkerCompleted);
  connect(worker_, &LaserWorker::RequestFailed, this, &LaserController::OnWorkerFailed);
  thread_.start();
}
void LaserController::StopWorkerAndWait() {
  if (!thread_.isRunning()) return;
  QMetaObject::invokeMethod(worker_, &LaserWorker::Shutdown, Qt::BlockingQueuedConnection);
  thread_.quit();
  thread_.wait();
  worker_ = nullptr;
}
void LaserController::Connect(int id, LaserConnection connection) {
  if (disconnect_request_ || scan_request_) {
    emit RequestFailed(id, {{}, "Connect laser", "Controller is reserved."}); return;
  }
  EnsureWorkerStarted();
  auto* worker = worker_.data();
  QMetaObject::invokeMethod(worker, [=] { worker->Connect(id, connection); }, Qt::QueuedConnection);
}
void LaserController::Disconnect(int id) {
  if (!worker_) { emit RequestCompleted(id); return; }
  if (disconnect_request_ || scan_request_) {
    emit RequestFailed(id, {{}, "Disconnect laser", "Controller is reserved."}); return;
  }
  disconnect_request_ = id;
  auto* worker = worker_.data();
  QMetaObject::invokeMethod(worker, [=] { worker->Disconnect(id); }, Qt::QueuedConnection);
}
void LaserController::SetOutputEnabled(int id, bool enabled) {
  if (!worker_ || disconnect_request_ || (id > 0 && scan_request_)) {
    OnWorkerFailed(id, {{}, "Laser output", "Controller is disconnected or reserved."}); return;
  }
  auto* worker = worker_.data();
  QMetaObject::invokeMethod(worker, [=] { worker->SetOutputEnabled(id, enabled); }, Qt::QueuedConnection);
}
void LaserController::SetOutputForScan(bool enabled) {
  if (scan_request_) {
    emit ScanOperationFailed({{}, "Laser output", "An operation is already pending."}); return;
  }
  const int id = NextScanRequestId();
  if (!id) { emit ScanOperationFailed({{}, "Laser output", "Request IDs exhausted."}); return; }
  scan_request_ = id;
  scan_cancelled_ = false;
  SetOutputEnabled(id, enabled);
}
void LaserController::CancelScanOperation() {
  if (scan_request_) scan_cancelled_ = true;
}
void LaserController::OnWorkerCompleted(int id) {
  if (id > 0) {
    if (disconnect_request_ == id) {
      StopWorkerAndWait();
      disconnect_request_.reset();
    }
    emit RequestCompleted(id);
  } else if (scan_request_ == id) {
    scan_request_.reset();
    if (std::exchange(scan_cancelled_, false)) emit ScanOperationCancelled();
    else emit ScanOperationCompleted();
  }
}
void LaserController::OnWorkerFailed(int id, OperationError error) {
  if (id >= 0) {
    if (disconnect_request_ == id) disconnect_request_.reset();
    emit RequestFailed(id, error);
  } else if (scan_request_ == id) {
    scan_request_.reset();
    scan_cancelled_ = false;
    emit ScanOperationFailed(error);
  }
}
}  // namespace application