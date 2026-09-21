#include "kinesis_simulation.h"

#include "Thorlabs.MotionControl.KCube.DCServo.h"

namespace thorlabs {

KinesisSimulation::KinesisSimulation(bool simulation) : simulation_(simulation)
{
    if (simulation_) {
        TLI_InitializeSimulations();
    }
}

KinesisSimulation::~KinesisSimulation()
{
    if (simulation_) {
        TLI_UninitializeSimulations();
    }
}

} // namespace thorlabs
