#include <RotaryEncoder.h>


#define PIN_IN1 2
#define PIN_IN2 3
#define button 12
#define LED1 8
#define LED2 7
#define LED3 6

uint8_t LED = 0;
uint8_t buttonState, lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
boolean techChosen = false;

RotaryEncoder *encoder = nullptr;

void checkPosition() {
  encoder->tick(); 
}



void setup() {
  Serial.begin(9600);

  encoder = new RotaryEncoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);
  attachInterrupt(digitalPinToInterrupt(PIN_IN1), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_IN2), checkPosition, CHANGE);

  pinMode(button, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  LEDchoice();
}

void loop() {

  if (!techChosen) {
    rotaryEncoder();

    if (buttonPress()) {
      Serial.print("You have chosen ");
      LEDchoice();
      techChosen = true;
    }
  }

  if (buttonPress() && techChosen) {
    Serial.print("You are cancelling ");
    LEDchoice();
    techChosen = false;
  }
}

boolean buttonPress(void) {
  int reading = digitalRead(button);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (!buttonState) {
        lastButtonState = reading;
        return true;
      }
    }
  }
  lastButtonState = reading;
  return false;
}

void rotaryEncoder() {
  static int pos = 0;

  encoder->tick();  

  int newPos = encoder->getPosition();
  if (pos != newPos) {
    if ((int)(encoder->getDirection()) < 0) {
      LED++;
      LEDchoice();
    } else {
      LED--;
      LEDchoice();
    }
    pos = newPos;
  }
}

void LEDchoice() {
  switch (LED) {
    case 0:
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      Serial.println("CV");
      break;
    case 1:
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, HIGH);
      digitalWrite(LED3, LOW);
      Serial.println("DPV");
      break;
    case 2:
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, HIGH);
      Serial.println("CA");
      break;
  }
}