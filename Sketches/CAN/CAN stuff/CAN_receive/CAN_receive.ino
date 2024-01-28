#include <SPI.h>
#include "mcp2515_can.h"

#define CAN_DATA_COUNT    0x03
#define CAN_2515

const int SPI_CS_PIN = 10;

mcp2515_can CAN(SPI_CS_PIN); // Set CS pin

void setup() {

    Serial.begin(115200);
    delay(2000);

    while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
        Serial.println("CAN init fail, retry...");
        delay(100);
    }
    Serial.println("CAN init ok!");
}


void loop() {
  
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
    }
}


//END FILE