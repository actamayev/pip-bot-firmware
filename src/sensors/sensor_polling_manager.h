#pragma once
#include "sensors/imu.h"
#include "utils/singleton.h"
#include "sensors/side_tof_manager.h"
#include "sensors/multizone_tof_sensor.h"

class SensorPollingManager : public Singleton<SensorPollingManager> {
    friend class Singleton<SensorPollingManager>;

    //This class will control when sensors are being polled.
    // For starting polling:
    //1. DONE: upon startup
    //2. When user goes into sandbox
    //3. DONE: Upon connecting to USB/WS
    // 4. While a program is running in the sandbox

    // When it should stop polling:
    // When receiving a message from Serial to stop (when exiting sandbox)
    // When disconnecting from WS
    // When disconnecting from USB
    // After 1 minute of inactivity (timeout)

    public:
        SensorPollingManager() = default;

        void startPolling();
        void stopPolling();
        bool isPolling() const { return polling; }
        
        // Call this method regularly in the main loop
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
