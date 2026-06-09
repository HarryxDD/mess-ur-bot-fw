#pragma once
#include <stdint.h>

// Sent from arm ESP32 -> base ESP32 over ESP-NOW.
typedef struct {
    float upperPitch;
    float upperRoll; 
    float forePitch; 
    float elbowAngle;
    float jerk;      
} ArmPacket;