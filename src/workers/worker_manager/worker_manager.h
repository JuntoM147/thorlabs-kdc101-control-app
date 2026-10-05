#ifndef WORKERS_WORKER_MANAGER_H_
#define WORKERS_WORKER_MANAGER_H_

#include <QThread>
#include <array>

#include "workers/laser_worker/laser_worker.h"
#include "workers/motor_worker/motor_worker.h"
#include "workers/scan_worker/scan_worker.h"

namespace workers {

// Owns thread lifetime. Device connections are requested separately by the UI.
class WorkerManager {
 public:
  WorkerManager();
  ~WorkerManager();
  WorkerManager(const WorkerManager&) = delete;
  WorkerManager& operator=(const WorkerManager&) = delete;
  void Shutdown();

  std::array<MotorWorker*, 3> motors{};
  LaserWorker* laser = nullptr;
  ScanWorker* scan = nullptr;

 private:
  std::array<QThread, 3> motor_threads_;
  QThread laser_thread_;
  QThread scan_thread_;
  bool stopped_ = false;
};

}  // namespace workers
#endif  // WORKERS_WORKER_MANAGER_H_
