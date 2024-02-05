#include <SPI.h>
#include <EEPROM.h>
#include "mcp2515_can.h"

#define CAN_DATA_COUNT    0x03
#define CAN_2515
#define PIN_STEP D6
#define PIN_DIR D7

const int IDaddress = 0; //where the board id is located for CAN identification purposes
const int SPI_CS_PIN = 10;
byte stiffness = -1;
byte idNumber = -1;
int curSteps = 0;
int targetSteps = 0;


mcp2515_can CAN(SPI_CS_PIN); // Set CS pin

void setup() {
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  SerialInit();
  CANInit();
  IDInit();
  //EEPROM setup
  for (int i = 0; i < 20; i++){
    Serial.print("Address: ");
    Serial.print(i);
    Serial.print("  Value: ");
    Serial.println(EEPROM.read(i));
  }
  while(!CANReceiveStiffness());
  RotateMotor();
  CANSendSuccess();
  
}

void loop() {
  while(!CANReceiveStiffness());
  RotateMotor();
  CANSendSuccess();
}

void SerialInit() {
    Serial.begin(115200);
    while(!Serial);
}

void CANInit() {
    while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
      Serial.println("CAN init fail, retry...");
      delay(100);
    }
    Serial.println("CAN init ok!");
}

void IDInit() {
  idNumber = EEPROM.read(IDaddress);
}

void CANSend(byte input) {
    uint8_t data_count[] = { CAN_DATA_COUNT, (input >> 16) & 0xFF, (input >> 8) & 0xFF, input & 0xFF };
    // send data:  id = 0x70, standard frame, data len = 8, stmp: data buf
    CAN.sendMsgBuf(0x70, 0, 4, data_count);       
}

bool CANReceiveStiffness() {
  unsigned char len = 0;
  unsigned char rxBuf[8];
  byte CANdata = 0;
  if (CAN_MSGAVAIL == CAN.checkReceive()) {         // check if data coming
    CAN.readMsgBuf(&len, rxBuf);    // read data,  len: data length, buf: data buf
    unsigned long canId = CAN.getCanId();
    if (len < 1) { return false; }
    for(int i=3; i < len; i++) {
      CANdata = rxBuf[i];
    }
    CANParseStiffness(CANdata);
    return true;
    }
  return false;
}

void CANParseStiffness (byte CANdata) {
  if((CANdata >> 4) == idNumber) {
    if(((CANdata & 0b1111) >= 0) && ((CANdata & 0b1111) <= 10)) {
      stiffness = CANdata & 0b1111;
      targetSteps = stiffness * 800;
      Serial.print("Stiffness = ");
      Serial.println(stiffness);
    }
    else {
      Serial.println("Invalid Stiffness");
    }

  }
}

void CANSendSuccess() {
  CANSend(idNumber);
}

void RotateMotor() {
  while(curSteps != targetSteps) {
    if(curSteps < targetSteps) {
        digitalWrite(PIN_DIR, LOW);
        digitalWrite(PIN_STEP, HIGH);
        delay(1);
        digitalWrite(PIN_STEP, LOW);
        delay(5);
        curSteps++;
    }
    else if(curSteps > targetSteps) {
        digitalWrite(PIN_DIR, HIGH);
        digitalWrite(PIN_STEP, HIGH);
        delay(1);
        digitalWrite(PIN_STEP, LOW);
        delay(5);      
        curSteps--;
    }
  }



}

//END FILE