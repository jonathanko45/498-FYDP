#include <SPI.h>
#include "mcp2515_can.h"

#define PIN_STEP 7
#define PIN_DIR 8
#define CAN_DATA_COUNT    0x03
#define CAN_2515

const int SPI_CS_PIN = 10;
const int CAN_INT_PIN = 2;

float currentKnobPosDeg = 180;
float desiredKnobPosDeg = currentKnobPosDeg;
int direction = 1;
const float msr = 8;
const float motorPotGearRatio = 8;
const float motorKnobGearRatio = 2;
const float potRotationRangeDeg = 315;

void UpdatePos();
void MotorStep();
void CheckValidPotPositon();
float GetKnobPosFromPotDeg();
void PositionCalibration();

mcp2515_can CAN(SPI_CS_PIN); // Set CS pin

void CAN_receiveSetup() {
    pinMode(PIN_STEP, OUTPUT);
    pinMode(PIN_DIR, OUTPUT);

    while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
        Serial.println("CAN init fail, retry...");
        delay(100);
    }
    Serial.println("CAN init ok!");
}


void CAN_receiveLoop() {
    unsigned char len = 0;
    unsigned char rxBuf[8];
    int input = 5;

    if (CAN_MSGAVAIL == CAN.checkReceive()) {         // check if data coming
        CAN.readMsgBuf(&len, rxBuf);    // read data,  len: data length, buf: data buf

        unsigned long canId = CAN.getCanId();

        Serial.println("-----------------------------");
        Serial.println("get data from ID: 0x");

         if (len < 1) { return; }


    for(int i=3; i < len; i++)
    {
      Serial.print(rxBuf[i]);
      input = rxBuf[i];
    }
    if(input == 0)
      desiredKnobPosDeg = 0;
    if(input == 1)
      desiredKnobPosDeg = 90;
    if(input == 2)
      desiredKnobPosDeg = 180;
    if(input == 3)
      desiredKnobPosDeg = 270;
    if(input == 4)
      desiredKnobPosDeg = 360;
    if(input == 5)
      desiredKnobPosDeg = 450;
    if(input == 6)
      desiredKnobPosDeg = 540;
    if(input == 7)
      desiredKnobPosDeg = 630;
    if(input == 8)
      desiredKnobPosDeg = 720;
    if(input == 9)
      desiredKnobPosDeg = 810;

    UpdatePos();

    }
}

void MotorStep() {

  digitalWrite(PIN_STEP, HIGH);
  delay(1);
  digitalWrite(PIN_STEP, LOW);
  delay(1);
}

void UpdatePos() {
  int currentPosMicroStep = (currentKnobPosDeg*motorKnobGearRatio/1.8)*msr;
  int desiredPosMicroStep = (desiredKnobPosDeg*motorKnobGearRatio/1.8)*msr;

  if(currentPosMicroStep != desiredPosMicroStep) {
    if(desiredPosMicroStep > currentPosMicroStep) {
      digitalWrite(PIN_DIR, HIGH);
      direction = 1;
    }
    else if(desiredPosMicroStep < currentPosMicroStep) {
      digitalWrite(PIN_DIR, LOW);
      direction = -1;
    }
    for(int i = 0; i < abs(desiredPosMicroStep - currentPosMicroStep); i++) {
      MotorStep();
    }
    currentPosMicroStep = desiredPosMicroStep;
    currentKnobPosDeg = ((currentPosMicroStep/msr)*1.8)/motorKnobGearRatio;
  }
}
