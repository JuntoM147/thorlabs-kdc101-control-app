#ifndef APPLICATION_LASER_CONTROLLER_H_
#define APPLICATION_LASER_CONTROLLER_H_

#include <optional>

#include <QObject>
#include <QPointer>
#include <QThread>

#include "../application_types/application_types.h"
#include "../laser_worker/laser_worker.h"

namespace application {

// Manages a worker thread to control the laser hardware
class LaserController : public QObject {
  Q_OBJECT

 public:
  explicit LaserController(QObject* parent = nullptr);
  ~LaserController() override;

 public slots:
  void Connect(int id, LaserConnection connection);
  void Disconnect(int id);
  void SetOutputEnabled(int id, bool enabled);

  // ID-free scan API. Application reserves access; one operation at a time.
  void SetOutputForScan(bool enabled);
  // A synchronous SDK write cannot be interrupted: settle it, then report cancelled.
  void CancelScanOperation();

 signals:
  void StateChanged(application::LaserState state);
  void RequestCompleted(int id);
  void RequestFailed(int id, application::OperationError error);

  // Only private scan IDs produce these signals, once the write has settled.
  // Retire IDs before emitting; a cancelled write must not also report completion.
  void ScanOperationCompleted();
  void ScanOperationFailed(application::OperationError error);
  void ScanOperationCancelled();

 private slots:
  void OnWorkerCompleted(int id);
  void OnWorkerFailed(int id, OperationError error);

 private:
  // Positive IDs belong to manual callers. Allocate negative scan IDs without
  // reuse; at INT_MIN mark exhaustion instead of overflowing. Zero is reserved.
  // Never forward retired negative IDs as completions. Preserve positive-ID
  // replies for Application; unsolicited device faults must still be reported.
  [[nodiscard]] int NextScanRequestId();
  void EnsureWorkerStarted();
  void StopWorkerAndWait();
  std::optional<int> disconnect_request_;

  int next_scan_request_id_ = -1;
  std::optional<int> scan_request_;
  bool scan_cancelled_ = false;
  QThread thread_;
  QPointer<LaserWorker> worker_;
};

}  // namespace application

#endif  // APPLICATION_LASER_CONTROLLER_H_
