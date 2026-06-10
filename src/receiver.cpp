#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include "shared.h"

Servo servoShoulder;
Servo servoSweep;   
Servo servoElbow;   

const int PIN_SHOULDER = 25;
const int PIN_SWEEP    = 26;
const int PIN_ELBOW    = 27;

// Ranges
const float UPPER_PITCH_MIN = -25, UPPER_PITCH_MAX = 80;
const float UPPER_ROLL_MIN  = -55, UPPER_ROLL_MAX  = 15;
const float ELBOW_MIN       = -85, ELBOW_MAX       = 20;

// const int SHOULDER_OUT_MIN = 40, SHOULDER_OUT_MAX = 140;
// const int SWEEP_OUT_MIN    = 50,  SWEEP_OUT_MAX    = 130;
// const int ELBOW_OUT_MIN    = 30,  ELBOW_OUT_MAX    = 150;

const int SHOULDER_US_MIN = 1100, SHOULDER_US_MAX = 1900;
const int SWEEP_US_MIN    = 1150, SWEEP_US_MAX    = 1850;
const int ELBOW_US_MIN    = 1050, ELBOW_US_MAX    = 1950;

const int PWM_MIN = 500, PWM_MAX = 2400;
float posShoulderUs = 1450, posSweepUs = 1450, posElbowUs = 1450;
int targetShoulderUs = 1450, targetSweepUs = 1450, targetElbowUs = 1450;
const float SMOOTH = 0.15f;
unsigned long lastUpdate = 0;

volatile bool newData = false;
ArmPacket received;

// int mapMicroseconds(float v, float inMin, float inMax) {
//     float out = (v - inMin) * (PWM_MAX - PWM_MIN) / (inMax - inMin) + PWM_MIN;
//     return constrain((int)out, PWM_MIN, PWM_MAX);
// }

int mapMicroseconds(float v, float inMin, float inMax, int usMin, int usMax) {
    float out = (v - inMin) * (usMax - usMin) / (inMax - inMin) + usMin;
    return constrain((int)out, usMin, usMax);
}

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
    if (len == sizeof(ArmPacket)) {
        memcpy(&received, data, sizeof(received));
        newData = true;
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    WiFi.mode(WIFI_STA);
    Serial.print("Receiver MAC: "); Serial.println(WiFi.macAddress()); // COPY this into RECEIVER_MAC in sender.cpp

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    servoShoulder.setPeriodHertz(50);
    servoSweep.setPeriodHertz(50);
    servoElbow.setPeriodHertz(50);
    servoShoulder.attach(PIN_SHOULDER, 500, 2400);
    servoSweep.attach(PIN_SWEEP, 500, 2400);
    servoElbow.attach(PIN_ELBOW, 500, 2400);

    servoShoulder.writeMicroseconds((int)posShoulderUs);
    servoSweep.writeMicroseconds((int)posSweepUs);
    servoElbow.writeMicroseconds((int)posElbowUs);

    if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW init failed"); while (1) delay(10); }
    esp_now_register_recv_cb(onRecv);

    Serial.println("Receiver ready - waiting for packets");
}

void loop() {
    if (newData) {
        newData = false;
        Serial.printf("RX  uPitch=%.1f uRoll=%.1f fPitch=%.1f elbow=%.1f\n",
                      received.upperPitch, received.upperRoll,
                      received.forePitch, received.elbowAngle);

        targetShoulderUs = mapMicroseconds(received.upperPitch, UPPER_PITCH_MIN, UPPER_PITCH_MAX, SHOULDER_US_MIN, SHOULDER_US_MAX);
        targetSweepUs    = mapMicroseconds(received.upperRoll,  UPPER_ROLL_MIN,  UPPER_ROLL_MAX, SWEEP_US_MIN, SWEEP_US_MAX);
        targetElbowUs    = mapMicroseconds(received.elbowAngle, ELBOW_MIN,       ELBOW_MAX, ELBOW_US_MIN, ELBOW_US_MAX);
    }

    if (millis() - lastUpdate >= 20) { // 50Hz update rate
        lastUpdate = millis();

        // Ease current position toward target (exponential smoothing)
        posShoulderUs += SMOOTH * (targetShoulderUs - posShoulderUs);
        posSweepUs    += SMOOTH * (targetSweepUs    - posSweepUs);
        posElbowUs    += SMOOTH * (targetElbowUs    - posElbowUs);

        servoShoulder.writeMicroseconds((int)posShoulderUs);
        servoSweep.writeMicroseconds((int)posSweepUs);
        servoElbow.writeMicroseconds((int)posElbowUs);
    }
}