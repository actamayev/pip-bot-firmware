#ifndef SENSOR_SETUP_H
#define SENSOR_SETUP_H

class SensorSetup {
    public:
        void sensor_setup();
    
    private:
        bool setupTofSensors();  
};

extern SensorSetup sensorSetup;

#endif
