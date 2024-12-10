#include "./include/utils.h"

void quaternionToEuler(float qr, float qi, float qj, float qk, float& yaw, float& pitch, float& roll) {
    // Roll (x-axis rotation)
    float sinr_cosp = 2 * (qr * qi + qj * qk);
    float cosr_cosp = 1 - 2 * (qi * qi + qj * qj);
    roll = atan2(sinr_cosp, cosr_cosp);

    // Pitch (y-axis rotation)
    float sinp = 2 * (qr * qj - qk * qi);
    if (abs(sinp) >= 1)
        pitch = copysign(M_PI / 2, sinp); // use 90 degrees if out of range
    else
        pitch = asin(sinp);

    // Yaw (z-axis rotation)
    float siny_cosp = 2 * (qr * qk + qi * qj);
    float cosy_cosp = 1 - 2 * (qj * qj + qk * qk);
    yaw = atan2(siny_cosp, cosy_cosp);
    
    // Convert to degrees
    yaw *= RAD_TO_DEG;
    pitch *= RAD_TO_DEG;
    roll *= RAD_TO_DEG;
}
