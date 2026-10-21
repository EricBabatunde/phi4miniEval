```cpp

#include <Arduino.h>


enum ValveState { IDLE, EXTENDING, RETRACTING };

class ValveController {

private:

  ValveState state = IDLE;

  unsigned long lastUpdateTime = 0;

  const unsigned long EXTENDING_DURATION = 500;

  const unsigned long RETRACTING_DURATION = 300;


public:


  bool updateValveState() {

    unsigned long currentTime = millis();


    switch (state) {

      case IDLE:

        if (currentTime - lastUpdateTime >= EXTENDING_DURATION) {

          state = EXTENDING;

          lastUpdateTime = currentTime;

          return true;

        }

        break;

      case EXTENDING:

        if (currentTime - lastUpdateTime >= EXTENDING_DURATION) {

          state = RETRACTING;

          lastUpdateTime = currentTime;

          return true;

        }

        break;

      case RETRACTING:

        if (currentTime - lastUpdateTime >= RETRACTING_DURATION) {

          state = IDLE;

          lastUpdateTime = currentTime;

          return true;

        }

        break;

    }

    return false;

  }

};


void setup() {

  Serial.begin(9600);

}


void loop() {

  ValveController valveController;


  if (valveController.updateValveState()) {

    switch (valveController.state) {

      case IDLE:

        Serial.println("Valve is idle");

        break;

      case EXTENDING:

        Serial.println("Valve is extending");

        break;

      case RETRACTING:

        Serial.println("Valve is retracting");

        break;

    }

  }

}

```
