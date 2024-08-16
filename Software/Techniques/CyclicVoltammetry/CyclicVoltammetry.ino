#include <LMP91000.h>
#include <Wire.h>

LMP91000 pstat = LMP91000();

uint16_t refVolt = 3300;
uint16_t range = 6;
uint8_t resolution = 10;
uint8_t bias = 13;            //number of voltage steps
uint8_t cycles = 3;           //number of full cycles i.e. 2 segments
uint8_t cycle_direction = 1;  //1 = positive, 0 = negative
uint16_t rate = 200;
uint8_t settling_time = 50;

int16_t initE = 500;
int16_t finalE = -200;

void setup() {
  Wire.begin();
  Serial.begin(9600);

  delay(50);
  pstat.standby();
  delay(50);
  pstat.disableFET();
  pstat.setGain(3);
  pstat.setRLoad(1);
  pstat.setExtRefSource();
  pstat.setIntZ(1);
  pstat.setThreeLead();
  delay(2000);  //warm-up time for the gas sensor
  Serial.println("Ready");
}

void loop() {
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    if (inChar == 'a') {
      cycle();
    }
  }
}


void cycle() {
  initE = determineLMP91000Bias(initE);
  finalE = determineLMP91000Bias(finalE);
  if (initE < finalE) {
    for (int i = 0; i < cycles; i++) {
      int16_t lowE = initE;
      while (lowE <= finalE) {
        if (lowE <= 0) {
          pstat.setNegBias();
          pstat.setBias(abs(lowE));
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[abs(lowE)] * -1);
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
          delay(rate);
          lowE++;
        } else {
          pstat.setPosBias();
          pstat.setBias(lowE);
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[lowE]);
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
          delay(rate);
          lowE++;
        }
      }
      while (lowE > initE) {
        if (lowE <= 0) {
          pstat.setNegBias();
          pstat.setBias(abs(lowE));
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[abs(lowE)] * -1);
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
          delay(rate);
          lowE--;
        } else {
          pstat.setPosBias();
          pstat.setBias(lowE);
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[lowE]);
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
          delay(rate);
          lowE--;
        }
      }
    }
  } else {
    for (int i = 0; i < cycles; i++) {
      int16_t highE = initE;
      while (highE >= finalE) {
        if (highE <= 0) {
          pstat.setNegBias();
          pstat.setBias(abs(highE));
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[abs(highE)] * -1);
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
          delay(rate);
          highE--;
        } else {
          pstat.setPosBias();
          pstat.setBias(highE);
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[highE]);
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
          delay(rate);
          highE--;
        }
      }
      while (highE < initE) {
        if (highE <= 0) {
          pstat.setNegBias();
          pstat.setBias(abs(highE));
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[abs(highE)] * -1);
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
          delay(rate);
          highE++;
        } else {
          pstat.setPosBias();
          pstat.setBias(highE);
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[highE]);
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
          delay(rate);
          highE++;
        }
      }
    }
  }
  pstat.setBias(0);
}

void positive_cycle() {
  pstat.setPosBias();
  for (int i = 0; i <= bias; i++) {
    pstat.setBias(i);
    //delay(50);
    delay(settling_time);
    Serial.print(refVolt * TIA_BIAS[i]);
    Serial.print(",");
    delay(1);
    Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
    delay(rate);
  }
  for (int i = bias - 1; i > 0; i--) {
    pstat.setBias(i);
    //delay(50);
    delay(settling_time);
    Serial.print(refVolt * TIA_BIAS[i]);
    Serial.print(",");
    delay(1);
    Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
    delay(rate);
  }
}

void negative_cycle() {
  pstat.setNegBias();
  for (int i = 0; i <= bias; i++) {
    pstat.setBias(i);
    //delay(50);
    delay(settling_time);
    Serial.print(refVolt * TIA_BIAS[i]);
    Serial.print(",");
    delay(1);
    Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
    delay(rate);
  }
  for (int i = bias - 1; i > 0; i--) {
    pstat.setBias(i);
    //delay(50);
    delay(settling_time);
    Serial.print(refVolt * TIA_BIAS[i]);
    Serial.print(",");
    delay(1);
    Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
    delay(rate);
  }
}

signed char determineLMP91000Bias(int16_t voltage) {
  signed char polarity = 0;
  if (voltage < 0) {
    polarity = -1;
  } else {
    polarity = 1;
  }

  int16_t v1 = 0;
  int16_t v2 = 0;

  voltage = abs(voltage);

  if (voltage == 0) {
    return 0;
  }

  for (int i = 0; i < NUM_TIA_BIAS - 1; i++) {
    v1 = refVolt * TIA_BIAS[i];
    v2 = refVolt * TIA_BIAS[i + 1];

    if (voltage == v1) {
      return polarity * i;
    } else if (voltage > v1 && voltage < v2) {
      if (abs(voltage - v1) < abs(voltage - v2)) {
        return polarity * i;
      } else {
        return polarity * (i + 1);
      }
    }
  }
  return 0;
}