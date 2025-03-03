#pragma once
#include <Arduino.h>

class PIDController {
    private:
        float kp;           // Proportional gain
        float ki;           // Integral gain
        float kd;           // Derivative gain

        float setpoint;     // Target value
        float lastError;    // Last error for derivative calculation
        float integral;     // Accumulated error for integral calculation
        float output;       // Last calculated output
        float outputMin;    // Minimum output limit
        float outputMax;    // Maximum output limit

        unsigned long lastTime;   // Last update time

    public:
        PIDController(float p, float i, float d, float min, float max)
            : kp(p), ki(i), kd(d), outputMin(min), outputMax(max) {
            reset();
        }

        // Reset the controller state
        void reset() {
            setpoint = 0.0;
            lastError = 0.0;
            integral = 0.0;
            output = 0.0;
            lastTime = millis();
        }

        // Set the target value
        void setSetpoint(float target) {
            // Reset integral when changing setpoint to prevent integral windup
            if (setpoint != target) {
                integral = 0.0;
            }
            setpoint = target;
        }

        // Update the PID controller and return the output
        float compute(float input) {
            // Calculate time delta
            unsigned long now = millis();
            float deltaTime = (now - lastTime) / 1000.0; // Convert to seconds
            
            // Skip update if no time has passed
            if (deltaTime <= 0.0) {
                return output;
            }
            
            // Calculate error
            float error = setpoint - input;
            
            // Calculate integral with anti-windup
            integral += error * deltaTime;
            
            // Calculate derivative
            float derivative = 0.0;
            if (deltaTime > 0.0) {
                derivative = (error - lastError) / deltaTime;
            }
            
            // Calculate output
            output = kp * error + ki * integral + kd * derivative;
            
            // Apply output limits
            if (output > outputMax) {
                output = outputMax;
                // Anti-windup - prevent integral from growing when output is saturated
                integral -= error * deltaTime;
            } else if (output < outputMin) {
                output = outputMin;
                // Anti-windup - prevent integral from growing when output is saturated
                integral -= error * deltaTime;
            }
            
            // Update state for next call
            lastError = error;
            lastTime = now;
            
            return output;
        }
        
        // Get the last computed output without recalculating
        float getOutput() const {
            return output;
        }
        
        // Adjust PID parameters
        void setTunings(float p, float i, float d) {
            kp = p;
            ki = i;
            kd = d;
        }
};
