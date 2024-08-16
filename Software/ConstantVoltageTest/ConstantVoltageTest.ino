#include <LMP91000.h>
#include <Wire.h>

LMP91000 pstat = LMP91000();

uint8_t bias = 13;            //number of voltage steps
uint8_t cycles = 3;           //number of full cycles i.e. 2 segments
uint8_t cycle_direction = 0;  //1 = positive, 0 = negative
uint16_t rate = 200;
uint8_t settling_time = 50;
uint8_t a,pn;

void setup() {
  Wire.begin();
  Serial.begin(9600);

  delay(50);
  pstat.standby();
  delay(50);
  pstat.disableFET();
  pstat.setGain(3);
  pstat.setRLoad(0);
  pstat.setExtRefSource();
  pstat.setIntZ(1);
  pstat.setThreeLead();
  delay(2000);  //warm-up time for the gas sensor
}

void loop() {
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    if (inChar == 'a') {
      a++;
      set(pn, a);
    } if (inChar == 's'){
      a--;
      set(pn, a);
    } if(inChar == 'n'){
      pn = 0;
      set(pn, a);
    } if (inChar == 'p'){
      pn = 1;
      set(pn, a);
    }
  }
}

void set(uint8_t pn, uint8_t i) {
  if (pn) {
    pstat.setPosBias();
  } else{
    pstat.setNegBias();
  }
  pstat.setBias(i);
  delay(settling_time);
  Serial.println(i);
}