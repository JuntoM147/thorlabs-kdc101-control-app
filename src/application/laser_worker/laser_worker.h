#ifndef APPLICATION_LASER_WORKER_H_
#define APPLICATION_LASER_WORKER_H_

#include <memory>

#include <QObject>

#include "../application_types/application_types.h"
#include "../../devices/ni_daq_laser/ni_daq_laser.h"

namespace application {

// Dedicated thread to control the laser hardware
class LaserWorker : public QObject {
  Q_OBJECT

 public:
  explicit LaserWorker(QObject* parent = nullptr);
  ~LaserWorker() override;

 public slots:
  void Connect(int id, LaserConnection connection);
  void Disconnect(int id);
  void SetOutputEnabled(int id, bool enabled);
  void Shutdown();

 signals:
  void StateChanged(application::LaserState state);
  void RequestCompleted(int id);
  void RequestFailed(int id, application::OperationError error);
  void ShutdownReady();

 private:
  std::unique_ptr<NI_DAQ::Laser> laser_; // owns the RAII laser wrapper
  LaserState state_;
  bool shutting_down_ = false;
};

}  // namespace application

#endif  // APPLICATION_LASER_WORKER_H_
