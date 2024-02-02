// demo: CAN-BUS Shield, send data
#include <SPI.h>

#define CAN_DATA_COUNT    0x03
#define CAN_2515

const int SPI_CS_PIN = 10;
const int CAN_INT_PIN = 2;

#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN); // Set CS pin


int input = 0;                           

void CAN_sendSetup() {

    while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
        // SERIAL_PORT_MONITOR.println("CAN init fail, retry...");
        Serial.println("CAN init fail, retry...");
        delay(100);
    }
    // SERIAL_PORT_MONITOR.println("CAN init ok!");
    Serial.println("CAN init ok!");
}


void CAN_sendLoop() {
    if(Serial.available()){
    // SERIAL_PORT_MONITOR.print("CAN SEND Count data = ");
    Serial.print("CAN SEND Count data = ");
     input = Serial.read() - '0';
        //  SERIAL_PORT_MONITOR.println( input);
         Serial.println( input);
    uint8_t data_count[] = { CAN_DATA_COUNT, (input >> 16) & 0xFF, (input >> 8) & 0xFF, input & 0xFF };
    // send data:  id = 0x70, standard frame, data len = 8, stmp: data buf
    CAN.sendMsgBuf(0x70, 0, 4, data_count);     
    }     
}
