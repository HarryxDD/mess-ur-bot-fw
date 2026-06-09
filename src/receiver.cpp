#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include "shared.h"

Servo servoShoulder;
Servo servoSweep;   
Servo servoElbow;   

const int PIN_SHOULDER = 13;
const int PIN_SWEEP    = 12;
const int PIN_ELBOW    = 14;

// Ranges
const float UPPER_PITCH_MIN = -70, UPPER_PITCH_MAX = 70;
const float UPPER_ROLL_MIN  = -40, UPPER_ROLL_MAX  = 20;
const float ELBOW_MIN       = -40, ELBOW_MAX       = 110;

const int SERVO_MIN = 0, SERVO_MAX = 180;

float posShoulder = 90, posSweep = 90, posElbow = 90;
const float SMOOTH = 0.15f;

volatile bool newData = false;
ArmPacket received;

int mapAngle(float v, float inMin, float inMax) {
    float out = (v - inMin) * (SERVO_MAX - SERVO_MIN) / (inMax - inMin) + SERVO_MIN;
    return constrain((int)out, SERVO_MIN, SERVO_MAX);
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

    servoShoulder.write((int)posShoulder);
    servoSweep.write((int)posSweep);
    servoElbow.write((int)posElbow);

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

        int targetShoulder = mapAngle(received.upperPitch, UPPER_PITCH_MIN, UPPER_PITCH_MAX);
        int targetSweep    = mapAngle(received.upperRoll,  UPPER_ROLL_MIN,  UPPER_ROLL_MAX);
        int targetElbow    = mapAngle(received.elbowAngle, ELBOW_MIN,       ELBOW_MAX);

        // Ease current position toward target (exponential smoothing)
        posShoulder += SMOOTH * (targetShoulder - posShoulder);
        posSweep    += SMOOTH * (targetSweep    - posSweep);
        posElbow    += SMOOTH * (targetElbow    - posElbow);

        servoShoulder.write((int)posShoulder);
        servoSweep.write((int)posSweep);
        servoElbow.write((int)posElbow);

        Serial.printf("SH=%d SW=%d EL=%d\n",
                      (int)posShoulder, (int)posSweep, (int)posElbow);
    }
}