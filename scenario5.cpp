#include <Wire.h>
#include <stdint.h>
#include <chrono>

#define SENSOR_ADDR 0x48 // Example I2C address for the sensor
#define STATUS_REGISTER 0x01 // Example status register address
#define READY_BIT (1 << 0) // Assuming the ready bit is the least significant bit

bool initializeSensor() {
    Wire.begin();
    Wire.beginTransmission(SENSOR_ADDR);
    Wire.write(0x00); // Send soft-reset command
    Wire.endTransmission();

    auto startTime = std::chrono::steady_clock::now();
    bool ready = false;

    while (std::chrono::steady_clock::now() - startTime < std::chrono::milliseconds(50)) {
        Wire.requestFrom(SENSOR_ADDR, (uint8_t)1);
        if (Wire.available()) {
            uint8_t status = Wire.read();
            ready = (status & READY_BIT) > 0;
            if (ready) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Avoid busy waiting
    }

    return ready;
}

void setup() {
    Serial.begin(115200);
    if (initializeSensor()) {
        Serial.println("Sensor initialized successfully");
    } else {
        Serial.println("Sensor initialization failed: ready bit not set");
    }
}

void loop() {
    // Nothing to do here
}
