#include "config.h"
#include "wifi_manager.h"
#include "esp32_api_client.h"
#include "webserver_manager.h"
#include <Adafruit_VL53L1X.h>
#include <Wire.h>

// Create three sensor objects
Adafruit_VL53L1X sensor1 = Adafruit_VL53L1X();
// Adafruit_VL53L1X sensor2 = Adafruit_VL53L1X();
// Adafruit_VL53L1X sensor3 = Adafruit_VL53L1X();


struct SensorReading {
    int16_t distance;
    bool valid;
};

// Function to read from a single sensor
SensorReading readSensor(Adafruit_VL53L1X& sensor, const char* sensorName) {
    SensorReading reading = {0, false};

    if (sensor.dataReady()) {
        reading.distance = sensor.distance();
        reading.valid = (reading.distance != -1);

        if (reading.valid) {
            Serial.print(sensorName);
            Serial.print(": ");
            Serial.print(reading.distance);
            Serial.print("mm  ");
        } else {
            Serial.print(sensorName);
            Serial.print(": Error  ");
        }

        sensor.clearInterrupt();
    }

    return reading;
}

bool setupSensors() {
    // Initialize I2C
    Wire.begin(TIME_OF_FLIGHT_SDA, TIME_OF_FLIGHT_SCL);

    // Set all XSHUT pins as outputs
    pinMode(TIME_OF_FLIGHT_XSHUT_1, OUTPUT);
    pinMode(TIME_OF_FLIGHT_XSHUT_2, OUTPUT);
    pinMode(TIME_OF_FLIGHT_XSHUT_3, OUTPUT);

    // Disable all sensors
    digitalWrite(TIME_OF_FLIGHT_XSHUT_1, LOW);
    digitalWrite(TIME_OF_FLIGHT_XSHUT_2, LOW);
    digitalWrite(TIME_OF_FLIGHT_XSHUT_3, LOW);
    delay(10);

    // Initialize sensor 1 (keep default address)
    digitalWrite(TIME_OF_FLIGHT_XSHUT_1, HIGH);
    delay(10);
    if (!sensor1.begin(TOF_SENSOR1_ADDRESS, &Wire)) {
        Serial.println("Failed to boot first VL53L1X");
        return false;
    }
    sensor1.startRanging();

    // Initialize sensor 2
    digitalWrite(TIME_OF_FLIGHT_XSHUT_2, HIGH);
    // delay(10);
    // if (!sensor2.begin(TOF_SENSOR2_ADDRESS, &Wire)) {
    //     Serial.println("Failed to boot second VL53L1X");
    //     return false;
    // }
    // sensor2.startRanging();

    // // Initialize sensor 3
    digitalWrite(TIME_OF_FLIGHT_XSHUT_3, HIGH);
    // delay(10);
    // if (!sensor3.begin(TOF_SENSOR3_ADDRESS, &Wire)) {
    //     Serial.println("Failed to boot third VL53L1X");
    //     return false;
    // }
    // sensor3.startRanging();

    return true;
}

void setup() {
	Serial.begin(115200);
	// delay(3000); // Ensure time for Serial Monitor to connect
    while (!Serial) delay(10);

	// Setup onboard LED
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW); // Turn off LED initially

    // wifiManager.initializeWiFi();

    // // Try connecting to stored WiFi credentials
    // if (!wifiManager.connectToStoredWiFi()) {
    //     // If WiFi connection fails, start Access Point for WiFi configuration
    //     wifiManager.startAccessPoint();
    // } else {
    //     // If WiFi connection succeeds, initiate WebSocket connection
    //     apiClient.connectWebSocket();
    // }

    Serial.println("Starting VL53L1X multiple sensors test...");
  
    if (!setupSensors()) {
        Serial.println("Failed to initialize sensors!");
        while (1) delay(10);  // Halt if setup failed
    }
    
    Serial.println("All sensors initialized successfully!");
}

void loop() {
    // Handle DNS and Web Server requests (non-blocking)
    // webServerManager.handleClientRequests();

    // // Handle WebSocket events (if connected to WiFi)
    // if (WiFi.status() == WL_CONNECTED) {
    //     apiClient.pollWebSocket();
    // }
    SensorReading reading1 = readSensor(sensor1, "Sensor 1");
    // SensorReading reading2 = readSensor(sensor2, "Sensor 2");
    // SensorReading reading3 = readSensor(sensor3, "Sensor 3");
    
    Serial.println(); // New line after all readings
    
    delay(50);  // 20Hz reading rate
}
