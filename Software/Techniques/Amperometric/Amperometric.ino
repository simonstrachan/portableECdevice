#include <LMP91000.h>
#include <Wire.h>

LMP91000 pstat = LMP91000();

int16_t refVolt = 3300;   //milliVolts if working with a 3.3V device
uint8_t resolution = 10;  //10-bits
uint16_t pre_stepV = 0;
uint32_t quietTime = 1000;
uint16_t volt = 250;
uint32_t time = 5000;
uint16_t samples = 80;
uint8_t range = 6;

void setup() {
  Wire.begin();
  Serial.begin(9600);

  pstat.standby();
  delay(1000);  //warm-up time for the gas sensor

  runAmp();
}


void loop() {
}

void runAmp() {
  pstat.disableFET();
  pstat.setGain(3);
  pstat.setRLoad(0);
  pstat.setIntRefSource();
  pstat.setIntZ(1);
  pstat.setThreeLead();
  pstat.setBias(0);

  //Print column headers
  String current = "";
  if (range == 12) current = "Current(pA)";
  else if (range == 9) current = "Current(nA)";
  else if (range == 6) current = "Current(uA)";
  else if (range == 3) current = "Current(mA)";
  else current = "SOME ERROR";

  Serial.println("Voltage(mV),Time(ms)," + current);

  int16_t voltageArray[2] = { pre_stepV, volt };
  uint32_t timeArray[2] = { quietTime, time };

  for (uint8_t i = 0; i < 2; i++) {
    uint32_t fs = timeArray[i] / samples;
    voltageArray[i] = determineLMP91000Bias(voltageArray[i]);

    if (voltageArray[i] < 0) {
      pstat.setNegBias();
    } else {
      pstat.setPosBias();
    }
    unsigned long startTime = millis();
    pstat.setBias(abs(voltageArray[i]));

    while (millis() - startTime < timeArray[i]) {
      Serial.print((uint16_t)(refVolt * TIA_BIAS[abs(voltageArray[i])] * (voltageArray[i] / abs(voltageArray[i]))));
      Serial.print(",");
      Serial.print(millis());
      Serial.print(",");
      Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
      delay(fs);
    }
  }
  //End at 0V
  pstat.setBias(0);
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
