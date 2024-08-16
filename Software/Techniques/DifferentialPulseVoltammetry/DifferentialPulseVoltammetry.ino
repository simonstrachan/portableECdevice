#include <LMP91000.h>
#include <Wire.h>

LMP91000 pstat = LMP91000();

int16_t refVolt = 2500;   //milliVolts if working with a 3.3V device
uint8_t resolution = 10;  //10-bits
uint8_t range = 6;
uint8_t i = 0;

void setup() {
  Wire.begin();
  Serial.begin(9600);

  pstat.standby();
  pstat.disableFET();
  pstat.setGain(7);
  pstat.setRLoad(0);
  pstat.setIntRefSource();
  pstat.setIntZ(1);
  pstat.setThreeLead();
  pstat.setBias(0);
  delay(1000);  //warm-up time for the gas sensor

  DPV(-200, 450, 4, 50, 50, 16.7, 500);
}


void loop() {
}

void DPV(int16_t initE, int16_t finalE, int16_t incrE, int16_t amp, uint32_t pulse_width, uint32_t scanrate, uint32_t pulse_period) {

  //initialisation
  uint32_t lowPeriod = pulse_period - pulse_width;
  int16_t volt = initE;
  int32_t current = 0;
  int32_t currentArray[scanrate];

  //Print column headers
  String curHeader = "";
  if (range == 12) curHeader = "Current(pA)";
  else if (range == 9) curHeader = "Current(nA)";
  else if (range == 6) curHeader = "Current(uA)";
  else if (range == 3) curHeader = "Current(mA)";
  else curHeader = "ERROR";

  Serial.println("Voltage(mV), " + curHeader);

  //setting the voltage
  //while (volt <= finalE) {
  int volt_bias = determineLMP91000Bias(volt);
  while (volt <= finalE) {
    Serial.println(volt_bias);
    if (volt_bias < 0) {
      pstat.setNegBias();
    } else {
      pstat.setPosBias();
    }
    pstat.setBias(abs(volt_bias));

    volt = (refVolt * TIA_BIAS[abs(volt_bias)] * (volt_bias / abs(volt_bias)));
    /*
    Serial.print(millis());
    Serial.print(",");
    Serial.println(volt);
    */
    delay(pulse_period);
    /*
    Serial.print(millis());
    Serial.print(",");
    Serial.println(volt);
    */

    if (i % 2 == 0) {
      volt_bias += 2;
      for (int i = 0; i < scanrate; i++) {
        currentArray[i] = pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution);
      }
    } else {
      volt_bias--;
      for (int i = 0; i < scanrate; i++) {
        currentArray[i] -= pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution);
        current += currentArray[i];
      }
      current /= scanrate;
      Serial.print(volt);
      Serial.print(", ");
      Serial.println(abs(current));
    }
    i++;
  }
  /*
    //Low portion
    setVoltage(volt);
    Serial.print(millis());
    Serial.print(",");
    Serial.println((int16_t)(refVolt * TIA_BIAS[abs(determineLMP91000Bias(volt))] * (determineLMP91000Bias(volt) / abs(determineLMP91000Bias(volt)))));
    delay(lowPeriod);

    //1st sampling window
    for (int i = 0; i < scanrate; i++) {
      currentArray[i] = (pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution) * -1);
    }

    //High portion
    Serial.print(millis());
    Serial.print(",");
    Serial.println((int16_t)(refVolt * TIA_BIAS[abs(determineLMP91000Bias(volt))] * (determineLMP91000Bias(volt) / abs(determineLMP91000Bias(volt)))));
    volt += amp;
    setVoltage(volt);
    Serial.print(millis());
    Serial.print(",");
    Serial.println((int16_t)(refVolt * TIA_BIAS[abs(determineLMP91000Bias(volt))] * (determineLMP91000Bias(volt) / abs(determineLMP91000Bias(volt)))));
    delay(pulse_width);

    //2nd sampling window
    for (int i = 0; i < scanrate; i++) {
      currentArray[i] += pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution);
      current += currentArray[i];
    }

    current /= scanrate;
  /*
    Serial.print(millis());
    Serial.print(",");
    Serial.print((int16_t)(refVolt * TIA_BIAS[abs(determineLMP91000Bias(volt))] * (determineLMP91000Bias(volt) / abs(determineLMP91000Bias(volt)))));
    Serial.print(",");
    Serial.println(current);

    //step voltage
    Serial.print(millis());
    Serial.print(",");
    Serial.println((int16_t)(refVolt * TIA_BIAS[abs(determineLMP91000Bias(volt))] * (determineLMP91000Bias(volt) / abs(determineLMP91000Bias(volt)))));
    volt -= (amp - incrE);
  }*/

  //End at 0V
  pstat.setBias(0);
}

void setVoltage(int16_t v1) {
  int16_t v2 = determineLMP91000Bias(v1);
  if (v2 < 0) {
    pstat.setNegBias();
  } else {
    pstat.setPosBias();
  }
  pstat.setBias(abs(v2));
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