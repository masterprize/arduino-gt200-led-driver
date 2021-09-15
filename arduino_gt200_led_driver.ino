//#include "Arduino.h"
//TODO: power saving mode?
//#include <avr/sleep.h>

//main led pin (connected to management transistor)
int ledPin = 5; // pins may be 3,5,6,9,10,11
//current pwm index of array with values
volatile int current_index = 0;
int max_index; //array size, calc on setup

//pvm values, select next index on button press. Exprerimental values based on voltage/current measurement
int vals[] = {40, 76, 120, 170, 205, 0}; //255 may be not safe voltage, but measurement shows 0.7a actually
//switch mode button pin
const byte btn_pin = 2; //power saving pins: 2,3 for nano
//last button state
int lastState = LOW;  // the previous state from the input pin
//current button state
int currentState;

//pin for managing extra leds (look for infun gt200 flashlight)
int edge_leds = 3;

//analog pin to read values from voltage divider, connected to battery (2kOhm + 1kOhm)
int batPin = 1;

//full bat now is about 2.9-3v (on divider) - 610 on analog read, low bat is 2.47
//2.47 is 0.494 of 5v, 0.494*1023 = 505
const int BAT_MAX_STATE = 610;
const int LOW_BAT = 540;

//timer for flashing edge_leds. Meaning - low battery
long lowBatTime = 0;
//timer interval
const long LOW_BAT_INTERVAL = 600;

//current main led brightness (pwm value)
int brightness = 0;

//short press detection const
const long SHORT_PRESS_TIME = 1200; // 500 milliseconds
long pressedTime = 0;
long releasedTime = 0;

//flashing main led mode flag & vals/consts
bool flashing = false;
const long FLASHING_INTERVAL = 600;
long flashingTime = 0;

void setup() {
   pinMode(ledPin, OUTPUT); // sets the pin as output
   max_index = sizeof(vals) / sizeof(int);

   pinMode(btn_pin, INPUT_PULLUP);
   pinMode(edge_leds, OUTPUT);

   //set current main led mode (last index is zero - off)
   current_index = max_index - 1;
   analogWrite(ledPin, vals[current_index]);
}

void loop() {

  long now = millis();

  //check bat state
  int batState = analogRead(batPin);
  checkBatState(batState, now);

  //check button state
  checkButtonState(now);

  //updates main led state
  updateLedState(batState, now);

  // save the the last state
  lastState = currentState;
}

void checkBatState(int batState, long now) {
  if (batState < LOW_BAT) {
    //start flashing extra leds meaning low battery
    long diff = now - lowBatTime;
    if (lowBatTime == 0) {
      lowBatTime = now;
      analogWrite(edge_leds, 150);
    } else if (diff > LOW_BAT_INTERVAL && diff < LOW_BAT_INTERVAL * 2) {
      analogWrite(edge_leds, 0);
    } else if (diff > LOW_BAT_INTERVAL * 2) {
      lowBatTime = 0;
    }
  } else {
      //by default extra leds always on
      analogWrite(edge_leds, 150);
  }
}

void checkButtonState(long now) {

  // read the state of the switch/button:
  currentState = digitalRead(btn_pin);
  if (currentState != lastState) {

    if(lastState == LOW && currentState == HIGH)
      pressedTime = now;

    if(lastState == HIGH && currentState == LOW)
      releasedTime = now;

    long pressDuration = releasedTime - pressedTime;
    if( pressDuration > 0 && pressDuration < SHORT_PRESS_TIME ) {
       ++current_index;
      if (current_index >= max_index) {
        current_index = 0;
      }
      //turn off flashing main led mode
      flashing = false;
    }
  } else if (!flashing && currentState == HIGH && now - pressedTime > SHORT_PRESS_TIME) {
    //long pressing, turn on flashing main led mode
    flashing = true;
    flashingTime = now;
    if (vals[current_index] == 0) {
      current_index = 0;
    }
  }
}

void updateLedState(int batState, long now) {
  //updates main led state
  int nv = vals[current_index];
  if (flashing) {
    long fDiff = now - flashingTime;
    if (fDiff > FLASHING_INTERVAL && fDiff < 2 * FLASHING_INTERVAL) {
      nv = 0;
    } else if (fDiff >= 2 * FLASHING_INTERVAL) {
      flashingTime = now;
    }
  }
  if (nv != 0) {
    nv += (BAT_MAX_STATE - batState) / 4;
  }
  if (abs(brightness - nv) > 8) {
    brightness = nv;
    analogWrite(ledPin, brightness);
  }
}
