#include <SPI.h>

#define chipSelectPin 10
int inValue = 0;
double wiper = 0;
double lastWiper = -1;
double terminalA = 4600;

void setup() {
  // put your setup code here, to run once:
  pinMode(chipSelectPin, OUTPUT);
  Serial.begin(9600);
  SPI.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    inValue = Serial.read();
  }
  if (inValue == 'a') {
    wiper++;
    inValue = 0;
  } else if (inValue == 's') {
    wiper--;
    inValue = 0;
  }
  double voltage = wiper / 256;
  voltage *= terminalA;
  digitalPotWrite(wiper);
  if (wiper != lastWiper) {
    Serial.print(wiper);
    Serial.print(", ");
    Serial.println(voltage);
    lastWiper = wiper;
  }
}

void digitalPotWrite(int value) {
  digitalWrite(chipSelectPin, LOW);
  delay(100);
  SPI.transfer(value);
  delay(100);
  digitalWrite(chipSelectPin, HIGH);
}