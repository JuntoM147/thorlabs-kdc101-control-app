#ifndef DEVICES_PCIE6323_LASER_H_
#define DEVICES_PCIE6323_LASER_H_
#include <NIDAQmx.h>

#include <expected>
#include <memory>
#include <string>

#include "error/error.h"
namespace NI_DAQ {

using errors::Error;

class Pcie6323Laser {
 public:
  using CreateResult = std::expected<std::unique_ptr<Pcie6323Laser>, Error>;
  [[nodiscard]] static CreateResult CreateLaser(const std::string& channel);
  ~Pcie6323Laser() noexcept;
  Pcie6323Laser(const Pcie6323Laser&) = delete;
  Pcie6323Laser& operator=(const Pcie6323Laser&) = delete;
  Pcie6323Laser(Pcie6323Laser&&) = delete;
  Pcie6323Laser& operator=(Pcie6323Laser&&) = delete;

  [[nodiscard]] Error TurnOn();
  [[nodiscard]] Error TurnOff();

 private:
  Pcie6323Laser() = default;
  [[nodiscard]] Error Write(bool on);

  TaskHandle task_ = nullptr;
  bool configured_ = false;
};
}  // namespace NI_DAQ
#endif
