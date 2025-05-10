#pragma once
#include "./utils/singleton.h"
#include "./side_time_of_flight_sensor.h"

class SideTofManager : public Singleton<SideTofManager> {
    friend class Singleton<SideTofManager>;

    public:
        SideTimeOfFlightSensor leftSideTofSensor;
        SideTimeOfFlightSensor rightSideTofSensor;
};
