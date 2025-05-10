#pragma once
#include "./utils/singleton.h"
#include "./side_time_of_flight_sensor.h"

class SideTofManager : public Singleton<SideTofManager> {
    friend class Singleton<SideTofManager>;

    public:
        SideTimeOfFlightSensor leftSideTofSensor;
        SideTimeOfFlightSensor rightSideTofSensor;

        void turnOffSideTofs() {
            leftSideTofSensor.turnSensorOff();
            rightSideTofSensor.turnSensorOff();
        }

        SideTofCounts getBothSideTofCounts() {
            uint16_t leftCounts = leftSideTofSensor.getCounts();
            uint16_t rightCounts = rightSideTofSensor.getCounts();

            return {
                leftCounts,
                rightCounts
            };
        }
};
