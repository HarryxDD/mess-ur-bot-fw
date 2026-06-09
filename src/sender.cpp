#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <MPU9250.h>
#include "shared.h"

// uint8_t RECEIVER_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast MAC address: 64:B7:08:CE:B6:34
uint8_t RECEIVER_MAC[] = {0x64, 0xB7, 0x08, 0xCE, 0xB6, 0x34};

MPU9250 mpuUpper(Wire, 0x68);
MPU9250 mpuFore(Wire1, 0x68); // Using Wire1 I2C bus instead

constexpr int I2C0_SDA = 21;
constexpr int I2C0_SCL = 22;
constexpr int I2C1_SDA = 19;
constexpr int I2C1_SCL = 18;

ArmPacket packet;

float upperPitchEst = 0, upperRollEst = 0, forePitchEst = 0;
unsigned long lastMicros = 0;
const float ALPHA = 0.98f;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL_MS = 100;

float accelPitch(float ax, float ay, float az) {
    return atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
}
float accelRoll(float ax, float ay, float az) {
    return atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
}

bool startIMU(MPU9250 &mpu, const char *name) {
    int status = mpu.begin();
    if (status < 0) {
        Serial.printf("%s init failed (status %d)\n", name, status);
        return false;
    }
    mpu.setAccelRange(MPU9250::ACCEL_RANGE_8G);
    mpu.setGyroRange(MPU9250::GYRO_RANGE_500DPS);
    mpu.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_41HZ);
    mpu.setSrd(19);
    return true;
}

void setup() {
	Serial.begin(115200);
    delay(500);

    Wire.begin(I2C0_SDA, I2C0_SCL);
    Wire.setClock(400000); // 400kHz fast mode

    Wire1.begin(I2C1_SDA, I2C1_SCL);
    Wire1.setClock(400000); // 400kHz fast mode

    if (!startIMU(mpuUpper, "Upper IMU")) while (1) delay(10);
    if (!startIMU(mpuFore,  "Fore IMU"))  while (1) delay(10);

    mpuUpper.readSensor();
    upperPitchEst = accelPitch(mpuUpper.getAccelX_mss(), mpuUpper.getAccelY_mss(), mpuUpper.getAccelZ_mss());
    upperRollEst = accelRoll(mpuUpper.getAccelX_mss(), mpuUpper.getAccelY_mss(), mpuUpper.getAccelZ_mss());
    mpuFore.readSensor();
    forePitchEst = accelPitch(mpuFore.getAccelX_mss(), mpuFore.getAccelY_mss(), mpuFore.getAccelZ_mss());

    WiFi.mode(WIFI_STA);
    Serial.print("Sender MAC: "); Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) { 
        Serial.println("ESP-NOW init failed"); while (1) delay(10); 
    }

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, RECEIVER_MAC, 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) { 
        Serial.println("Add peer failed"); while (1) delay(10); 
    }

    lastMicros = micros();
    Serial.println("Sender ready");
}

void loop() {
    unsigned long now = micros();
    float dt = (now - lastMicros) / 1e6f;
    lastMicros = now;

    // Upperarm IMU
    mpuUpper.readSensor();
    float uAx = mpuUpper.getAccelX_mss(), uAy = mpuUpper.getAccelY_mss(), uAz = mpuUpper.getAccelZ_mss();
    float uGy = mpuUpper.getGyroY_rads() * 180.0 / PI;   // pitch-axis rate, deg/s
    float uGx = mpuUpper.getGyroX_rads() * 180.0 / PI;   // roll-axis rate, deg/s
    upperPitchEst = ALPHA * (upperPitchEst + uGy * dt) + (1 - ALPHA) * accelPitch(uAx, uAy, uAz);
    upperRollEst  = ALPHA * (upperRollEst  + uGx * dt) + (1 - ALPHA) * accelRoll (uAx, uAy, uAz);

    // Forearm IMU
    mpuFore.readSensor();
    float fAx = mpuFore.getAccelX_mss(), fAy = mpuFore.getAccelY_mss(), fAz = mpuFore.getAccelZ_mss();
    float fGy = mpuFore.getGyroY_rads() * 180.0 / PI;
    forePitchEst = ALPHA * (forePitchEst + fGy * dt) + (1 - ALPHA) * accelPitch(fAx, fAy, fAz);

    if (millis() - lastSend >= SEND_INTERVAL_MS) {
        lastSend = millis();
        packet.upperPitch = upperPitchEst;
        packet.upperRoll  = upperRollEst;
        packet.forePitch  = forePitchEst;
        packet.elbowAngle = forePitchEst - upperPitchEst;
        packet.jerk       = 0;

        esp_now_send(RECEIVER_MAC, (uint8_t *)&packet, sizeof(packet));

        Serial.printf("uPitch=%.1f uRoll=%.1f fPitch=%.1f elbow=%.1f\n",
                      packet.upperPitch, packet.upperRoll, packet.forePitch, packet.elbowAngle);
    }
}