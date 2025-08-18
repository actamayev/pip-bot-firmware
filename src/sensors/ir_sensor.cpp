// #include "utils/config.h"
// #include "ir_sensor.h"

// IrSensor::IrSensor() {
//     // Initialize motor pins
//     pinMode(PIN_MUX_A0, OUTPUT);
//     pinMode(PIN_MUX_A1, OUTPUT);
//     pinMode(PIN_MUX_A2, OUTPUT);
    
//     // Configure multiplexer output pin as analog input
//     pinMode(PIN_MUX_OUT, INPUT);
//     analogReadResolution(12);
    
//     // Configure and enable IR sensor
//     pinMode(PIN_IR_EN, OUTPUT);
//     analogWrite(PIN_IR_EN, 200); // 51/255 = 20% duty cycle
// }

// void IrSensor::read_ir_sensor() {
//     for (uint8_t i = 0; i < 5; i++) {
//         // Set multiplexer to current channel
//         setMuxChannel(channels[i].A0, channels[i].A1, channels[i].A2);
        
//         // Read sensor value and convert to voltage
//         uint16_t rawValue = analogRead(PIN_MUX_OUT);
//         sensorReadings[i] = (rawValue * 3.3) / 4095.0;
//     }
// }

// void IrSensor::setMuxChannel(bool A0, bool A1, bool A2) {
//     digitalWrite(PIN_MUX_A0, A0);
//     digitalWrite(PIN_MUX_A1, A1);
//     digitalWrite(PIN_MUX_A2, A2);
//     vTaskDelay(pdMS_TO_TICKS(3));  // Small delay for multiplexer to settle
// }

// bool IrSensor::shouldBePolling() const {
//     ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
//     return timeouts.shouldEnableIr();
// }

// void IrSensor::updateSensorData() {
//     // Check if we should enable/disable the sensor based on timeouts
//     ReportTimeouts& timeouts = SensorDataBuffer::getInstance().getReportTimeouts();
//     bool shouldEnable = timeouts.shouldEnableIr();
    
//     if (shouldEnable && !sensorEnabled) {
//         enableIrSensor();
//     } else if (!shouldEnable && sensorEnabled) {
//         disableIrSensor();
//         return; // Don't try to read data if sensor is disabled
//     }
    
//     if (!sensorEnabled) return; // Skip if sensor not enabled
    
//     // Rate limiting - only read if enough time has passed
//     unsigned long currentTime = millis();
//     if (currentTime - lastUpdateTime < DELAY_BETWEEN_READINGS) {
//         return;
//     }
    
//     // Read current sensor data
//     read_ir_sensor();
//     lastUpdateTime = currentTime;
    
//     // Create IrData structure and write to buffer
//     IrData irData;
//     for (int i = 0; i < 5; i++) {
//         irData.sensorReadings[i] = sensorReadings[i];
//     }
//     irData.isValid = true;
//     irData.timestamp = currentTime;
    
//     // Write to buffer
//     SensorDataBuffer::getInstance().updateIrData(irData);
// }

// void IrSensor::enableIrSensor() {
//     if (sensorEnabled) return;
    
//     sensorEnabled = true;
//     SerialQueueManager::getInstance().queueMessage("IR sensors enabled");
// }

// void IrSensor::disableIrSensor() {
//     if (!sensorEnabled) return;
    
//     sensorEnabled = false;
//     SerialQueueManager::getInstance().queueMessage("IR sensors disabled due to timeout");
// }
