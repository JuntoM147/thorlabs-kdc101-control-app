#ifndef APPLICATION_SCAN_CONTROLLER_H_
#define APPLICATION_SCAN_CONTROLLER_H_

#include <array>
#include <cstddef>
#include <expected>
#include <functional>
#include <optional>

#include <QObject>
#include <QTimer>

#include "../application_types/application_types.h"
#include "../laser_controller/laser_controller.h"
#include "../motor_controller/motor_controller.h"
#include "../../algo/instructions/instructions.h"

namespace application {

class ScanController : public QObject {
  Q_OBJECT

 public:
  ScanController(MotorController& x, MotorController& y, MotorController& z, LaserController& laser, QObject* parent = nullptr);
  ~ScanController() override;

  [[nodiscard]] std::expected<void, OperationError> Configure(ScanConfiguration configuration);

 public slots:
  void Start();
  void Pause();
  void Resume();
  void Cancel();

 signals:
  void StateChanged(application::ScanState state);
  void ScanCompleted();
  void ScanCancelled();
  void ScanFailed(application::OperationError error);
  void RequestFailed(application::OperationError error);

 private slots:
  void ExecuteNextInstruction();
  void OnExposureFinished();

  void OnMotorCompleted(Axis axis);
  void OnMotorFailed(Axis axis, OperationError error);
  void OnMotorCancelled(Axis axis);
  void OnLaserCompleted();
  void OnLaserFailed(OperationError error);
  void OnLaserCancelled();
  void OnOperationTimedOut();

 private:
  enum class Device { kX, kY, kZ, kLaser };

  void OperationCompleted(Device device);
  void OperationFailed(Device device, OperationError error);
  void OperationCancelled(Device device);
  void FinishInstruction();

  void TurnOutputOffForPause();
  void StopMotorsAndTurnOutputOff();
  void FinishScan();
  void FailScan(OperationError error);
  void ContinueStopAndOff();
  void AwaitDevice(Device device);
  bool cleanup_started_ = false;  // Distinguishes draining an instruction from cleanup.

  // Borrowed devices, Application owns their lifetime
  std::array<std::reference_wrapper<MotorController>, 3> motors_;
  LaserController& laser_;

  // Instruction execution.
  algo::Program program_;
  ScanConfiguration configuration_;
  ScanState state_;  // Phase and progress; completed_instructions is the index.
  std::size_t substep_ = 0;  // X then Y, or the next stop/OFF operation.
  std::optional<Device> pending_device_;  // Device whose result we await.
  std::array<AxisState, 3> observations_{};
  std::optional<double> pending_start_position_;
  std::optional<double> pending_distance_;
  QTimer* exposure_timer_ = nullptr;  // QObject child, finishes a timed instruction
  QTimer* operation_timer_ = nullptr;  // QObject child, detects missing device replies

  bool program_output_enabled_ = false;  // Output to restore after pause
  bool cancelled_ = false;  // report scan cancelled or failed after clean up
  std::optional<OperationError> failure_error_;
};

}  // namespace application

#endif  // APPLICATION_SCAN_CONTROLLER_H_
