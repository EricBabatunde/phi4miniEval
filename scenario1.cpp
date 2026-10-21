#include <Arduino.h>

// State enum
enum ValveState { IDLE, EXTENDING, RETRACTING };

// Function to update the valve state
bool updateValveState(ValveState &currentState, unsigned long &lastMillis, unsigned long &cycleMillis, unsigned long cycleDuration, unsigned long interval) {
  unsigned long currentMillis = millis();
  
  switch (currentState) {
    case IDLE:
      if (currentMillis - lastMillis >= interval) {
        currentState = EXTENDING;
        lastMillis = currentMillis;
        return true; // Cycle completed
      }
      return false; // Still in IDLE state
    case EXTENDING:
      if (currentMillis - lastMillis >= 500) { // 500ms for extending
        currentState = RETRACTING;
        lastMillis = currentMillis;
        return true; // Cycle completed
      }
      return false; // Still extending
    case RETRACTING:
      if (currentMillis - lastMillis >= 300) { // 300ms for retracting
        currentState = IDLE;
        lastMillis = currentMillis;
        return true; // Cycle completed
      }
      return false; // Still retracting
  }
  return false; // Unknown state
}

void setup() {
  // Initialize serial for debugging
  Serial.begin(9600);
  ValveState currentState = IDLE;
  unsigned long lastMillis = 0;
  unsigned long cycleMillis = 0;
  unsigned long interval = 0;
}

void loop() {
  // Example loop to demonstrate the valve update
  if (updateValveState(currentState, lastMillis, cycleMillis, 1000, 500)) {
    Serial.println("Cycle completed. Starting new cycle.");
    cycleMillis = 0; // Reset cycleMillis after a complete cycle
  } else {
    Serial.println("Valve state: " + String(currentState));
  }
  
  // Small delay to simulate main loop iteration
  delay(100);
}
