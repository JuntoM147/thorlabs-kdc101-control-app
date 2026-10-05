#ifndef WORKERS_LASER_WORKER_H_
#define WORKERS_LASER_WORKER_H_

#include <QObject>
#include <QString>
#include <memory>

#include "workers/worker_types.h"

namespace NI_DAQ {
class Laser;
}

namespace workers {

// Owns the laser in its QThread. Submit requests through queued connections.
class LaserWorker final : public QObject {
  Q_OBJECT

 public:
  explicit LaserWorker(QObject* parent = nullptr);
  ~LaserWorker() override;

 public slots:
  // A successful connection leaves the output off.
  void ConnectDevice(workers::RequestId id, QString output_channel);
  void DisconnectDevice(workers::RequestId id);
  void SetOutput(workers::RequestId id, bool enabled);
  // Attempt laser-off before releasing the device and quitting the thread.
  void Shutdown();

 signals:
  void RequestFinished(workers::RequestId id, errors::Error result);
  void StateChanged(workers::LaserState state);
  void ShutdownFailed(errors::Error error);
  void ShutdownFinished();

 private:
  void FinishRequest(RequestId id, errors::Error result);
  bool RejectDuringShutdown(RequestId id);
  std::unique_ptr<NI_DAQ::Laser> laser_;
  LaserState state_;
  bool shutting_down_ = false;
};

}  // namespace workers

#endif  // WORKERS_LASER_WORKER_H_
