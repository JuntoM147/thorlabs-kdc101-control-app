#ifndef KINESIS_SIMULATION_H_
#define KINESIS_SIMULATION_H_

namespace thorlabs {

class KinesisSimulation {
 public:
    explicit KinesisSimulation(bool simulation = true);
    ~KinesisSimulation();

    KinesisSimulation(const KinesisSimulation&) = delete;
    KinesisSimulation& operator=(const KinesisSimulation&) = delete;
    KinesisSimulation(KinesisSimulation&&) = delete;
    KinesisSimulation& operator=(KinesisSimulation&&) = delete;

    [[nodiscard]] bool IsSimulation() const noexcept { return simulation_; }

 private:
    const bool simulation_;
};

} // namespace thorlabs

#endif // KINESIS_SIMULATION_H_
