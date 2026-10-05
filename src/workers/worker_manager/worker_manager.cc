#include "worker_manager.h"

#include <QEventLoop>

namespace workers {
namespace {
template <typename Worker>
Worker* StartWorker(QThread& thread, const QString& name) {
  auto* worker = new Worker;
  thread.setObjectName(name);
  worker->moveToThread(&thread);
  QObject::connect(worker, &Worker::ShutdownFinished, &thread, &QThread::quit,
                   Qt::DirectConnection);
  QObject::connect(&thread, &QThread::finished, worker, &QObject::deleteLater);
  thread.start();
  return worker;
}

template <typename Worker>
void StopWorker(Worker* worker, QThread& thread) {
  QEventLoop loop;
  QObject::connect(&thread, &QThread::finished, &loop, &QEventLoop::quit);
  QMetaObject::invokeMethod(worker, &Worker::Shutdown, Qt::QueuedConnection);
  loop.exec();
  thread.wait();
}
}  // namespace

void RegisterWorkerMetaTypes() {
  qRegisterMetaType<RequestId>("workers::RequestId");
  qRegisterMetaType<Axis>();
  qRegisterMetaType<MotorSettings>();
  qRegisterMetaType<MotorState>();
  qRegisterMetaType<LaserState>();
  qRegisterMetaType<ScanJob>();
  qRegisterMetaType<ScanProgress>();
  qRegisterMetaType<thorlabs::Direction>();
  qRegisterMetaType<thorlabs::StopMode>();
  qRegisterMetaType<errors::Error>();
}

WorkerManager::WorkerManager() {
  RegisterWorkerMetaTypes();
  motors[0] = StartWorker<MotorWorker>(motor_threads_[0], "X motor");
  motors[1] = StartWorker<MotorWorker>(motor_threads_[1], "Y motor");
  motors[2] = StartWorker<MotorWorker>(motor_threads_[2], "Z motor");
  laser = StartWorker<LaserWorker>(laser_thread_, "Laser");
  scan = StartWorker<ScanWorker>(scan_thread_, "Scan");
}

WorkerManager::~WorkerManager() { Shutdown(); }

void WorkerManager::Shutdown() {
  if (stopped_) return;
  stopped_ = true;
  // Scan cleanup requires live device workers to acknowledge its requests.
  StopWorker(scan, scan_thread_);
  StopWorker(laser, laser_thread_);
  for (std::size_t i = 0; i < motors.size(); ++i) {
    StopWorker(motors[i], motor_threads_[i]);
  }
}
}  // namespace workers
