#pragma once
#include "sensors/imu.h"
#include "utils/singleton.h"
#include "sensors/side_tof_manager.h"
#include "sensors/multizone_tof_sensor.h"

class SensorPollingManager : public Singleton<SensorPollingManager> {
    friend class Singleton<SensorPollingManager>;

    public:
        SensorPollingManager() = default;

        void startPolling();
        void stopPolling();
        bool isPolling() const { return polling; }
        
        void update();

    private:
        bool polling = false;
        unsigned long lastPollTime = 0;
        unsigned long pollingEndTime = 0;

        // Poll interval - 1 second
        static constexpr unsigned long POLL_INTERVAL_MS = 1000;

        // Polling duration - 1 minute
        static constexpr unsigned long POLLING_DURATION_MS = 60000;

        // Helper method to poll all sensors
        void pollSensors();
};
