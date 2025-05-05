#pragma once

#include "singleton.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "Arduino.h"
#include <vl53l7cx_class.h>

// Maximum message size
#define MAX_LOG_MESSAGE_SIZE 256
#define LOG_QUEUE_SIZE 20

typedef struct {
    char buffer[MAX_LOG_MESSAGE_SIZE];
} LogMessage_t;

class LogManager : public Singleton<LogManager> {
    friend class Singleton<LogManager>; // Allow Singleton to access private constructor

private:
    QueueHandle_t logQueue;
    TaskHandle_t logTaskHandle;
    
    // Private constructor
    LogManager() {
        // Create the message queue
        logQueue = xQueueCreate(LOG_QUEUE_SIZE, sizeof(LogMessage_t));
        
        // Create logging task
        xTaskCreatePinnedToCore(
            staticLogTask,
            "Logger",
            4096,          // Stack size - adjust if needed
            this,          // Pass 'this' as parameter
            1,             // Priority 
            &logTaskHandle,
            1              // Run on Core 1
        );
    }
    
    // Static task function to work with FreeRTOS
    static void staticLogTask(void* parameter) {
        // Cast parameter back to LogManager
        LogManager* self = static_cast<LogManager*>(parameter);
        self->logTask();
    }
    
    // Actual task implementation
    void logTask() {
        LogMessage_t message;
        
        for(;;) {
            // Wait for messages from the queue
            if(xQueueReceive(logQueue, &message, portMAX_DELAY) == pdTRUE) {
                // Write directly to Serial - this task has exclusive access
                Serial.print(message.buffer);
            }
        }
    }
    
    // Send a message to the log queue
    bool sendToLogQueue(const char* message) {
        if (logQueue == NULL) return false;
        
        LogMessage_t logMessage;
        strncpy(logMessage.buffer, message, MAX_LOG_MESSAGE_SIZE - 1);
        logMessage.buffer[MAX_LOG_MESSAGE_SIZE - 1] = '\0'; // Ensure null termination
        
        // Send to queue with a timeout - non-blocking with short timeout
        return xQueueSend(logQueue, &logMessage, pdMS_TO_TICKS(10)) == pdTRUE;
    }

public:
    // Simple logging methods
    void print(const char* message) {
        sendToLogQueue(message);
    }
    
    void println(const char* message) {
        char buffer[MAX_LOG_MESSAGE_SIZE];
        snprintf(buffer, MAX_LOG_MESSAGE_SIZE, "%s\n", message);
        sendToLogQueue(buffer);
    }
    
    // Method for formatting a complete ToF grid
    void logTofGrid(VL53L7CX_ResultsData* data, float avgDistance, bool objectDetected) {
        // We'll build the grid in chunks to fit our message size
        
        // Header
        this->println("VL53L7CX 8x8 Grid Distance Measurement");
        this->println("--------------------------------------");
        
        // Grid - row by row
        for (int row = 7; row >= 0; row--) {
            // Buffer for this row
            char rowBuffer[MAX_LOG_MESSAGE_SIZE];
            char* ptr = rowBuffer;
            
            // Separator line
            ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - rowBuffer), " ");
            for (int i = 0; i < 8; i++) {
                ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - rowBuffer), "-------- ");
            }
            ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - rowBuffer), "\n");
            
            // Send separator
            this->print(rowBuffer);
            
            // Distance values - reset buffer
            ptr = rowBuffer;
            ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - rowBuffer), "|");
            
            // Traverse columns from right to left (7 to 0) to fix horizontal flip
            for (int col = 7; col >= 0; col--) {
                // Calculate proper index in the data array
                int index = row * 8 + col;
                
                if (data->nb_target_detected[index] > 0) {
                    // Apply distance thresholds and signal quality filtering
                    uint16_t distance = data->distance_mm[index];
                    uint8_t status = data->target_status[index];
                    
                    // Filter out readings based on your thresholds
                    const uint16_t maxDistance = 4000;
                    const uint16_t minDistance = 10;
                    const uint8_t signalThreshold = 5;
                    
                    if (distance > maxDistance || distance < minDistance || status < signalThreshold) {
                        ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - rowBuffer), "    X   |");
                    } else {
                        ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - rowBuffer), " %4d mm|", distance);
                    }
                } else {
                    ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - rowBuffer), "    X   |");
                }
            }
            
            // Add newline
            ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - rowBuffer), "\n");
            
            // Send this row
            this->print(rowBuffer);
        }
        
        // Final separator line
        char footerBuffer[MAX_LOG_MESSAGE_SIZE];
        char* ptr = footerBuffer;
        ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE, " ");
        
        for (int i = 0; i < 8; i++) {
            ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - footerBuffer), "-------- ");
        }
        ptr += snprintf(ptr, MAX_LOG_MESSAGE_SIZE - (ptr - footerBuffer), "\n\n");
        this->print(footerBuffer);
        
        // Status information
        char statusBuffer[MAX_LOG_MESSAGE_SIZE];
        
        // Weighted average
        snprintf(statusBuffer, MAX_LOG_MESSAGE_SIZE, 
                "Weighted Average Distance: %s\n", 
                avgDistance > 0 ? String(avgDistance) + " mm" : "No valid readings");
        this->print(statusBuffer);
        
        // Object detection
        snprintf(statusBuffer, MAX_LOG_MESSAGE_SIZE,
                "Object Detected: %s\n\n",
                objectDetected ? "YES - OBSTACLE AHEAD" : "NO - PATH CLEAR");
        this->print(statusBuffer);
    }
};
