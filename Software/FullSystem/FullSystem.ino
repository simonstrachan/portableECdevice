#include <LMP91000.h>
#include <Wire.h>
#include <RotaryEncoder.h>
#include <SPI.h>

LMP91000 pstat = LMP91000();

//Pins
#define chipSelectPin 10
#define encoderPinA 2
#define encoderPinB 3
#define button 12
#define LED1 9
#define LED2 8
#define LED3 7
#define pLED 6
#define nLED 5

//VoltageSelector
const int16_t voltArray[] = { 450, 725, 1005, 1285, 1555, 1820, 2110, 2370, 2640, 2900, 3150, 3410, 3650, 3900, 4120 };  //measurements are for AD5160
const int8_t voltArray_length = 15;
const int16_t terminalA = 4600;

//UI variables
uint8_t LED = 0;
uint8_t buttonState, lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
boolean techChosen = false;
RotaryEncoder *encoder = nullptr;
/*
  Working settings for DPV:
  user_gain = 4;
  rload = 0;
  opVolt = 3300;
  refVolt = 1500;
  resolution = 10;
  range = 9;
*/
//Setting variables
uint8_t user_gain = 5;    //0 = external resistance, 1 = 2.75k, 2 = 3.5k, 3 = 7k, 4 = 14k, 5 = 35k, 6 = 120k, 7 = 350k;
uint8_t rload = 0;        //0  = 10, 1 = 33, 2 = 50, 4 = 100;
uint16_t opVolt = 3300;   //3v3 ref
uint16_t refVolt = 2211;  //milliVolts
uint8_t resolution = 10;  //10-bits
uint8_t range = 9;        //12 = picoamperes, 9 = nano, 6 = micro, 3 = milli

//CV variables
int16_t CV_initE = -200;
int16_t CV_finalE = 500;
int32_t CV_quietTime = 2000;
uint8_t cycles = 3;           //number of full cycles i.e. 2 segments
uint8_t cycle_direction = 1;  //1 = pos, 0 = neg
uint16_t CV_rate = 500;      // 4v/s = 10ms, 0.1v/s = 500ms
uint8_t settling_time = 50;

//i-t variables
int16_t pre_volt = 599;
int32_t it_quietTime = 300000;
int16_t volt = 400;
int32_t time = 0;
uint16_t it_rate = 500;  //V per s

//DPV variables
int16_t DPV_initE = -100;
int16_t DPV_finalE = 300;
int32_t DPV_quietTime = 2000;
uint32_t pulse_width = 17;
uint8_t scanrate = 1;
uint32_t pulse_period = 100;

void setup() {
  Wire.begin();
  SPI.begin();
  Serial.begin(9600);

  encoder = new RotaryEncoder(encoderPinA, encoderPinB, RotaryEncoder::LatchMode::TWO03);
  attachInterrupt(digitalPinToInterrupt(encoderPinA), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), checkPosition, CHANGE);

  pinMode(button, INPUT_PULLUP);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  delay(50);
  pstat.standby();
  delay(50);
  pstat.disableFET();
  pstat.setGain(user_gain);
  pstat.setRLoad(rload);
  pstat.setExtRefSource();
  pstat.setIntZ(1);
  pstat.setThreeLead();
  delay(2000);  //warm-up time for the gas sensor
  Serial.print("Gain: ");
  Serial.println(user_gain);
  Serial.print("Ref Volt: ");
  Serial.println(refVolt);
  Serial.print("Rload: ");
  Serial.println(rload);
  Serial.print("Scanrate: ");
  Serial.print(scanrate);
  Serial.print(", Pulse: ");
  Serial.println(pulse_period);
}

void loop() {
  if (!techChosen) {

    rotaryEncoder();
    if (buttonPress()) {
      Serial.print("You have chosen ");
      LEDchoice();
      techChosen = true;
      switch (LED) {
        case 0:
          CV();
          techChosen = false;
          break;
        case 1:
          DPV();
          techChosen = false;
          break;
        case 2:
          it();
          techChosen = false;
          break;
      }
    }
    /*
    if (Serial.available() > 0) {
      // read the incoming byte:
      LED = Serial.read();
      Serial.print("You have chosen ");
      LEDchoice();
      techChosen = true;
      switch (LED) {
        case 0:
          CV();
          techChosen = false;
          break;
        case 1:
          DPV();
          techChosen = false;
          break;
        case 2:
          it();
          techChosen = false;
          break;
      }
    }
    */
  }
}

void checkPosition() {
  encoder->tick();
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
      Serial.println("i-t");
      break;
  }
}

void CV() {
  //Print column headers
  String current = "";
  if (range == 12) current = "Current(pA)";
  else if (range == 9) current = "Current(nA)";
  else if (range == 6) current = "Current(uA)";
  else if (range == 3) current = "Current(mA)";
  else current = "SOME ERROR";

  Serial.println("Voltage(mV), Vout, " + current);
  CV_initE = determineLMP91000Bias(CV_initE);
  CV_finalE = determineLMP91000Bias(CV_finalE);

  pstat.setNegBias();
  pstat.setBias(abs(CV_initE));
  delay(CV_quietTime);
  if (CV_initE < CV_finalE) {
    for (int i = 0; i < cycles; i++) {
      int16_t lowE = CV_initE;
      while (lowE <= CV_finalE) {
        if (lowE <= 0) {
          pstat.setNegBias();
          pstat.setBias(abs(lowE));
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[abs(lowE)] * -1);
          Serial.print(",");
          delay(1);
          Serial.print(analogRead(A0));
          Serial.print(",");
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
          delay(CV_rate);
          lowE++;
        } else {
          pstat.setPosBias();
          pstat.setBias(lowE);
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[lowE]);
          Serial.print(",");
          delay(1);
          Serial.print(analogRead(A0));
          Serial.print(",");
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
          delay(CV_rate);
          lowE++;
        }
      }
      while (lowE > CV_initE) {
        if (lowE <= 0) {
          pstat.setNegBias();
          pstat.setBias(abs(lowE));
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[abs(lowE)] * -1);
          Serial.print(",");
          delay(1);
          Serial.print(analogRead(A0));
          Serial.print(",");
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
          delay(CV_rate);
          lowE--;
        } else {
          pstat.setPosBias();
          pstat.setBias(lowE);
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[lowE]);
          Serial.print(",");
          delay(1);
          Serial.print(analogRead(A0));
          Serial.print(",");
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
          delay(CV_rate);
          lowE--;
        }
      }
    }
  } else {
    for (int i = 0; i < cycles; i++) {
      int16_t highE = CV_initE;
      while (highE >= CV_finalE) {
        if (highE <= 0) {
          pstat.setNegBias();
          pstat.setBias(abs(highE));
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[abs(highE)] * -1);
          Serial.print(",");
          Serial.print(analogRead(A0));
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
          delay(CV_rate);
          highE--;
        } else {
          pstat.setPosBias();
          pstat.setBias(highE);
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[highE]);
          Serial.print(",");
          Serial.print(analogRead(A0));
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
          delay(CV_rate);
          highE--;
        }
      }
      while (highE < CV_initE) {
        if (highE <= 0) {
          pstat.setNegBias();
          pstat.setBias(abs(highE));
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[abs(highE)] * -1);
          Serial.print(",");
          Serial.print(analogRead(A0));
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
          delay(CV_rate);
          highE++;
        } else {
          pstat.setPosBias();
          pstat.setBias(highE);
          delay(settling_time);
          Serial.print(refVolt * TIA_BIAS[highE]);
          Serial.print(",");
          Serial.print(analogRead(A0));
          Serial.print(",");
          delay(1);
          Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
          delay(CV_rate);
          highE++;
        }
      }
    }
  }
  pstat.setBias(0);
}

void it() {
  //Print column headers
  String curHeader = "";
  if (range == 12) curHeader = "Current(pA)";
  else if (range == 9) curHeader = "Current(nA)";
  else if (range == 6) curHeader = "Current(uA)";
  else if (range == 3) curHeader = "Current(mA)";
  else curHeader = "SOME ERROR";

  Serial.println("Voltage(mV),Time(ms)," + curHeader);

  //int16_t voltageArray[2] = { pre_volt, volt };
  //int32_t timeArray[2] = { quietTime, time };

  unsigned long startTime = millis();
  /*
  for (uint8_t i = 0; i < 2; i++) {
    voltageArray[i] = determineLMP91000Bias(voltageArray[i]);
    Serial.print(voltageArray[i]);
    if (voltageArray[i] < 0) {
      pstat.setNegBias();
    } else {
      pstat.setPosBias();
    }
    unsigned long secondTime = millis();
    pstat.setBias(abs(voltageArray[i]));

    while (millis() - secondTime < timeArray[i]) {
      Serial.print(refVolt * TIA_BIAS[abs(voltageArray[i])] * (voltageArray[i] / abs(voltageArray[i])));
      Serial.print(",");
      Serial.print(millis() - startTime);
      Serial.print(",");
      Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution));
      delay(itrate);
    }
  }
  */
  int itvolt = determineLMP91000Bias(pre_volt);
  if (pre_volt < 0) {
    pstat.setNegBias();
  } else {
    pstat.setPosBias();
  }
  unsigned long secondTime = millis();
  pstat.setBias(abs(itvolt));

  while (millis() - secondTime < it_quietTime) {
    Serial.print(refVolt * TIA_BIAS[abs(itvolt)] * (itvolt / abs(itvolt)));
    Serial.print(",");
    Serial.print(millis() - startTime);
    Serial.print(",");
    Serial.println(pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution));
    delay(it_rate);
  }
  //End at 0V
  pstat.setBias(0);
}

int8_t determineLMP91000Bias(int16_t voltage) {
  int8_t polarity = 0;
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

void setVoltage(int16_t v1) {
  int16_t v2 = determineLMP91000Bias(v1);
  if (v2 < 0) {
    pstat.setNegBias();
  } else {
    pstat.setPosBias();
  }
  pstat.setBias(abs(v2));
}

void DPV() {
  //initialisation
  //uint32_t lowPeriod = pulse_period - pulse_width;
  int16_t volt = DPV_initE;
  int32_t current = 0;
  double currentArray[scanrate];
  uint8_t i = 0;
  double temp;

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
  int volt_bias = determineLMP91000Bias(volt) - 2;
  while (volt <= DPV_finalE) {
    double current1 = 0;
    double current2 = 0;
    double current3 = 0;
    if (volt_bias < 0) {
      pstat.setNegBias();
    } else {
      pstat.setPosBias();
    }
    pstat.setBias(abs(volt_bias));

    if (i == 0) {
      delay(DPV_quietTime);
    } else {
      //delay(lowPeriod);
      delay(pulse_period);
    }

    volt = (refVolt * TIA_BIAS[abs(volt_bias)] * (volt_bias / abs(volt_bias)));
    if (i % 2 == 0) {
      volt_bias += 2;
      for (int j = 0; j < scanrate; j++) {
        current1 += pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution);
        current1 /= scanrate;
      }
    } else {
      volt_bias--;
      for (int k = 0; k < scanrate; k++) {
        current2 += pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), opVolt / 1000.0, resolution);
        current2 /= scanrate;
        /*temp = pow(10, range) * pstat.getCurrent(pstat.getOutput(A0), refVolt / 1000.0, resolution);
        Serial.print(currentArray[k]);
        Serial.print(", ");
        Serial.print(temp);
        Serial.print(", ");
        currentArray[k] -= temp;
        Serial.print(currentArray[k]);
        Serial.print(", ");
        current += currentArray[k];
        Serial.println(current);*/
      }
      current2 -= current1;
      current3 = current2 * 0.5 + 0.00004;
      Serial.print(volt);
      Serial.print(", ");
      Serial.print(current2);
      Serial.print(", ");
      Serial.println(current3);
    }
    i++;
    if (volt_bias >= 14) {
      break;
    }
  }
  //End at 0V
  pstat.setBias(0);
}

int16_t findRefVoltage(int16_t voltDiv) {
  int16_t v1 = 0;
  int16_t v2 = 0;

  voltDiv = abs(voltDiv);

  if (voltDiv == 0) {
    return 0;
  }

  for (int i = 0; i < voltArray_length - 1; i++) {
    v1 = voltArray[i];
    v2 = voltArray[i + 1];

    if (voltDiv == v1) {
      setWiper(i);
      return voltArray[i];
    } else if (voltDiv > v1 && voltDiv < v2) {
      if (abs(voltDiv - v1) < abs(voltDiv - v2)) {
        setWiper(i);
        return voltArray[i];
      } else {
        setWiper(i + 1);
        return voltArray[i + 1];
      }
    }
  }
  return 0;
}

void setWiper(int8_t value) {
  digitalWrite(chipSelectPin, LOW);
  delay(100);
  SPI.transfer(value);
  delay(100);
  digitalWrite(chipSelectPin, HIGH);
}