#include <SPI.h>
#include "mcp2515_can.h"

#define CAN_DATA_COUNT    0x03
#define CAN_2515

const int SPI_CS_PIN = 7;
byte stiffness = -1;
boolean turn_done[4] = {false, false, false, false};

mcp2515_can CAN(SPI_CS_PIN); // Set CS pin

void CANsetup() {
  while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
    Serial.println("CAN init fail, retry...");
    delay(100);
  }
  Serial.println("CAN init ok!");
}

void CANSend(byte input) {
    uint8_t data_count[] = { CAN_DATA_COUNT, (input >> 16) & 0xFF, (input >> 8) & 0xFF, input & 0xFF };
    // send data:  id = 0x70, standard frame, data len = 8, stmp: data buf
    CAN.sendMsgBuf(0x70, 0, 4, data_count);       
}

bool CANReceiveSuccess() {
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
    if (CANdata >= 1 && CANdata <= 4){
      turn_done[CANdata - 1] = true;     
      return true;
    }
    else {
      Serial.println("Invalid MotorID");
    }
  }
  return false;
}

void SendStiffness(byte stiff, byte motorID) {
    CANSend((motorID << 4) + (stiff & 0b1111)); 
}

void allMotorsDone() {
  while (!(turn_done[0] == true && turn_done[1] == true && turn_done[2] == true && turn_done[3] == true)) {
    CANReceiveSuccess();
  }
  Serial.println("Calibration Success");  
}
