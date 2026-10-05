#include "algo/algorithm/algorithm.h"
#include "main_window.h"

namespace ui {

void MainWindow::SubmitMotor(
    Axis axis, std::string operation,
    std::function<void(workers::MotorWorker*, workers::RequestId)> command,
    std::optional<MotorSettings> settings, bool stopping) {
  const auto index = static_cast<std::size_t>(axis);
  if (!workers_available_ || !workers_ || index >= workers_->motors.size())
    return;
  if (controls_locked_) {
    ShowError({axis, operation, "Manual commands are disabled during a scan."});
    return;
  }
  for (const auto& [id, pending] : pending_requests_) {
    if (pending.axis == axis && (!stopping || pending.stopping)) return;
  }
  const auto id = next_request_id_++;
  pending_requests_.emplace(
      id, PendingRequest{axis, operation, settings, stopping});
  UpdatePendingRequests();
  auto* motor = workers_->motors[index];
  QMetaObject::invokeMethod(
      motor, [motor, id, command] { command(motor, id); },
      Qt::QueuedConnection);
}

void MainWindow::SubmitLaser(
    std::string operation,
    std::function<void(workers::LaserWorker*, workers::RequestId)> command) {
  if (!workers_available_ || !workers_ || controls_locked_) return;
  for (const auto& [id, pending] : pending_requests_) {
    if (!pending.axis) return;
  }
  const auto id = next_request_id_++;
  pending_requests_.emplace(
      id, PendingRequest{std::nullopt, operation, std::nullopt});
  UpdatePendingRequests();
  auto* laser = workers_->laser;
  QMetaObject::invokeMethod(
      laser, [laser, id, command] { command(laser, id); },
      Qt::QueuedConnection);
}

void MainWindow::UpdatePendingRequests() {
  for (int i = 0; i < 3; ++i) {
    const auto axis = static_cast<Axis>(i);
    bool pending = false, stopping = false;
    for (const auto& [id, request] : pending_requests_) {
      if (request.axis == axis) {
        pending = true;
        stopping |= request.stopping;
      }
    }
    emit AxisRequestsPending(axis, pending, stopping);
  }
  bool laser_pending = false;
  for (const auto& [id, request] : pending_requests_)
    laser_pending |= !request.axis;
  emit LaserRequestPending(laser_pending);
  SetManualRequestsPending(!pending_requests_.empty());
}

void MainWindow::FinishRequest(workers::RequestId id, errors::Error result) {
  const auto found = pending_requests_.find(id);
  if (found == pending_requests_.end())
    return;  // A scan result, not a manual request.
  const auto request = found->second;
  pending_requests_.erase(found);
  UpdatePendingRequests();
  if (!result.ok()) {
    ShowError({request.axis, request.operation, result.error_message()});
  } else if (request.axis && request.settings) {
    emit AxisSettingsApplied(*request.axis, *request.settings);
  }
  if (reset_requested_) {
    reset_failed_ |= !result.ok();
    if (pending_requests_.empty()) {
      reset_requested_ = false;
      SetControlsLocked(reset_failed_);
      if (!reset_failed_)
        OnScanResetCompleted();
      else {
        auto state = scan_state_;
        state.phase = ScanPhase::kFailed;
        UpdateScanState(state);
      }
    }
  }
}

void MainWindow::PublishMotorState(Axis axis, workers::MotorState state) {
  AxisState display;
  display.axis = axis;
  display.connection = state.connected ? ConnectionState::kConnected
                                       : ConnectionState::kDisconnected;
  display.position_mm = state.position_mm;
  if (state.status) {
    display.homed = state.status->homed;
    if (state.status->homing)
      display.operation = OperationState::kHoming;
    else if (state.status->jogging_forward || state.status->jogging_reverse)
      display.operation = OperationState::kJogging;
    else if (state.status->moving_forward || state.status->moving_reverse)
      display.operation = OperationState::kMoving;
  }
  emit AxisStateUpdated(display);
}

void MainWindow::ConnectMotorWorker(Axis axis, workers::MotorWorker* motor) {
  connect(
      this, &MainWindow::ConnectMotorRequested, this,
      [this, axis](MotorConnection connection) {
        if (connection.axis != axis) return;
        SubmitMotor(axis, "Connect motor", [connection](auto* motor, auto id) {
          motor->ConnectDevice(id,
                               QString::fromStdString(connection.serial_number),
                               connection.simulation);
        });
      });
  connect(this, &MainWindow::DisconnectMotorRequested, this,
          [this, axis](Axis requested) {
            if (requested == axis)
              SubmitMotor(axis, "Disconnect motor", [](auto* motor, auto id) {
                motor->DisconnectDevice(id);
              });
          });
  connect(this, &MainWindow::ConfigureAxisRequested, this,
          [this, axis](Axis requested, MotorSettings settings) {
            if (requested == axis)
              SubmitMotor(
                  axis, "Configure motor",
                  [settings](auto* motor, auto id) {
                    motor->Configure(id, settings);
                  },
                  settings);
          });
  connect(this, &MainWindow::HomeAxisRequested, this,
          [this, axis](Axis requested) {
            if (requested == axis)
              SubmitMotor(axis, "Home",
                          [](auto* motor, auto id) { motor->Home(id); });
          });
  connect(this, &MainWindow::MoveAxisRequested, this,
          [this, axis](Axis requested, double position) {
            if (requested == axis)
              SubmitMotor(axis, "Move", [position](auto* motor, auto id) {
                motor->MoveAbsolute(id, position);
              });
          });
  connect(this, &MainWindow::JogAxisRequested, this,
          [this, axis](Axis requested, Direction direction) {
            if (requested == axis)
              SubmitMotor(axis, "Jog", [direction](auto* motor, auto id) {
                motor->Jog(id, direction);
              });
          });
  connect(this, &MainWindow::DriveAxisRequested, this,
          [this, axis](Axis requested, Direction direction) {
            if (requested == axis)
              SubmitMotor(axis, "Drive", [direction](auto* motor, auto id) {
                motor->Drive(id, direction);
              });
          });
  connect(this, &MainWindow::StopAxisRequested, this,
          [this, axis](Axis requested, StopMode mode) {
            if (requested == axis)
              SubmitMotor(
                  axis, "Stop",
                  [mode](auto* motor, auto id) { motor->Stop(id, mode); },
                  std::nullopt, true);
          });
  connect(motor, &workers::MotorWorker::StateChanged, this,
          [this, axis](workers::MotorState state) {
            PublishMotorState(axis, state);
          });
  connect(motor, &workers::MotorWorker::SettingsChanged, this,
          [this, axis](workers::MotorSettings settings) {
            emit AxisSettingsUpdated(axis, settings);
          });
  connect(motor, &workers::MotorWorker::RequestFinished, this,
          [this](auto id, errors::Error result) { FinishRequest(id, result); });
  connect(motor, &workers::MotorWorker::PollingFailed, this,
          [this, axis](errors::Error error) {
            ShowError({axis, "Poll motor", error.error_message()});
          });
}

void MainWindow::ConnectScanAxis(workers::ScanWorker* scan, Axis axis,
                                 workers::MotorWorker* motor) {
  connect(
      scan, &workers::ScanWorker::MoveRequested, motor,
      [motor, axis](Axis requested, auto id, double distance) {
        if (requested == axis) motor->MoveRelative(id, distance);
      },
      Qt::QueuedConnection);
  connect(
      scan, &workers::ScanWorker::StopRequested, motor,
      [motor, axis](Axis requested, auto id, StopMode mode) {
        if (requested == axis) motor->Stop(id, mode);
      },
      Qt::QueuedConnection);
  connect(
      motor, &workers::MotorWorker::RequestFinished, scan,
      [scan, axis](auto id, errors::Error result) {
        scan->OnMotorFinished(axis, id, result);
      },
      Qt::QueuedConnection);
}

void MainWindow::ConnectLaserWorker(workers::LaserWorker* laser) {
  connect(this, &MainWindow::ConnectLaserRequested, this,
          [this](LaserConnection connection) {
            SubmitLaser("Connect laser", [connection](auto* laser, auto id) {
              laser->ConnectDevice(id, QString::fromStdString(
                                           connection.digital_output_channel));
            });
          });
  connect(this, &MainWindow::DisconnectLaserRequested, this, [this] {
    SubmitLaser("Disconnect laser",
                [](auto* laser, auto id) { laser->DisconnectDevice(id); });
  });
  connect(this, &MainWindow::SetLaserOutputRequested, this,
          [this](bool enabled) {
            SubmitLaser("Set laser output", [enabled](auto* laser, auto id) {
              laser->SetOutput(id, enabled);
            });
          });
  connect(laser, &workers::LaserWorker::StateChanged, this,
          [this](workers::LaserState state) {
            emit LaserStateUpdated({state.connected
                                        ? ConnectionState::kConnected
                                        : ConnectionState::kDisconnected,
                                    state.output_enabled});
          });
  connect(laser, &workers::LaserWorker::RequestFinished, this,
          [this](auto id, errors::Error result) { FinishRequest(id, result); });
  connect(laser, &workers::LaserWorker::ShutdownFailed, this,
          [this](errors::Error error) {
            ShowError({{}, "Shut down laser", error.error_message()});
          });
}

void MainWindow::ConnectScanWorker(workers::ScanWorker* scan) {
  ConnectScanAxis(scan, Axis::kX, workers_->motors[0]);
  ConnectScanAxis(scan, Axis::kY, workers_->motors[1]);
  auto* laser = workers_->laser;
  connect(scan, &workers::ScanWorker::LaserOutputRequested, laser,
          &workers::LaserWorker::SetOutput, Qt::QueuedConnection);
  connect(laser, &workers::LaserWorker::RequestFinished, scan,
          &workers::ScanWorker::OnLaserFinished, Qt::QueuedConnection);
  connect(this, &MainWindow::StartScanRequested, this,
          [this, scan](ScanConfiguration configuration) {
            if (!CanStartScan()) return;
            workers::ScanJob job;
            job.instructions = algo::GenerateInstructions(
                configuration.start_pixel, configuration.pattern,
                configuration.direction);
            job.pixel_size_mm = configuration.pixel_size_mm;
            job.pixel_exposure = configuration.exposure_time;
            SetControlsLocked(
                true);  // Lock immediately, before the queued Start executes.
            QMetaObject::invokeMethod(
                scan, [scan, job] { scan->Start(job); }, Qt::QueuedConnection);
          });
  connect(this, &MainWindow::PauseScanRequested, scan,
          &workers::ScanWorker::Pause, Qt::QueuedConnection);
  connect(this, &MainWindow::ResumeScanRequested, scan,
          &workers::ScanWorker::Resume, Qt::QueuedConnection);
  connect(this, &MainWindow::CancelScanRequested, scan,
          &workers::ScanWorker::Cancel, Qt::QueuedConnection);
  connect(this, &MainWindow::ResetScanRequested, this, [this, scan] {
    if (scan_state_.phase == ScanPhase::kIdle) {
      reset_requested_ = true;
      reset_failed_ = false;
      SubmitLaser("Reset laser",
                  [](auto* laser, auto id) { laser->SetOutput(id, false); });
      for (int i = 0; i < 3; ++i) {
        SubmitMotor(
            static_cast<Axis>(i), "Reset motor",
            [](auto* motor, auto id) { motor->Stop(id, StopMode::kProfiled); },
            std::nullopt, true);
      }
      SetControlsLocked(true);
      return;
    }
    reset_requested_ = true;
    QMetaObject::invokeMethod(scan, &workers::ScanWorker::Cancel,
                              Qt::QueuedConnection);
  });
  connect(scan, &workers::ScanWorker::ProgressChanged, this,
          &MainWindow::UpdateScanState);
  connect(scan, &workers::ScanWorker::Finished, this,
          [this](errors::Error result) {
            // Execution errors and cleanup errors are different. Unlock only
            // when the worker confirms successful cleanup by returning to Idle.
            const bool cleaned_up = scan_state_.phase == ScanPhase::kIdle;
            SetControlsLocked(!cleaned_up);
            auto state = scan_state_;
            state.phase = cleaned_up ? ScanPhase::kIdle : ScanPhase::kFailed;
            UpdateScanState(state);
            if (!result.ok() &&
                result.error_code() != errors::ErrorCode::kCancelled)
              ShowError({{}, "Scan", result.error_message()});
            if (reset_requested_ && cleaned_up) {
              reset_requested_ = false;
              OnScanResetCompleted();
            }
          });
}

void MainWindow::ConnectWorkers(workers::WorkerManager& workers) {
  if (workers_) return;  // Bind once.
  workers_ = &workers;
  for (int i = 0; i < 3; ++i)
    ConnectMotorWorker(static_cast<Axis>(i), workers.motors[i]);
  ConnectLaserWorker(workers.laser);
  ConnectScanWorker(workers.scan);
  SetWorkersAvailable(true);
}
}  // namespace ui
