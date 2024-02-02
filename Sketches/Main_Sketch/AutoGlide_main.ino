
/*******************************************************************************
 * Modified from PDQgrphicstest example of Arduino_GFX.
 * Run on Minima + 480x320 ILI9488 SPI TFT.
 ******************************************************************************/
//#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>

#define TFT_CS 10    //D10 or SS
#define TFT_RESET 4  //D4
#define TFT_DC 5     //D5
#define TFT_MOSI 11  //D11/MOSI
#define TFT_SCK 13   //D13/SCK
#define TFT_LED 3    //GPIO3
#define TFT_MISO -1  // not used for TFT

#define GFX_BL TFT_LED  // backlight pin

Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS);  // UNO
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, TFT_RESET, 3 /* rotation */, false /* IPS */);

/*******************************************************************************
 * CAN send 
 ******************************************************************************/
#include <SPI.h>

#define CAN_DATA_COUNT    0x03
#define CAN_2515

const int SPI_CS_PIN = 10;
const int CAN_INT_PIN = 2;

#ifdef CAN_2515
#include "mcp2515_can.h"
mcp2515_can CAN(SPI_CS_PIN); // Set CS pin
#endif

int input = 0;

/*******************************************************************************
 * CAN receive 
 ******************************************************************************/
#define PIN_STEP 7
#define PIN_DIR 8
#define CAN_DATA_COUNT    0x03
#define CAN_2515

float currentKnobPosDeg = 180;
float desiredKnobPosDeg = currentKnobPosDeg;
int direction = 1;
const float msr = 8;
const float motorPotGearRatio = 8;
const float motorKnobGearRatio = 2;
const float potRotationRangeDeg = 315;

float GetKnobPosFromPotDeg();

/*******************************************************************************
 * GPS and accelerometer 
 ******************************************************************************/
// GPS library
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// accelerometer libraries
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// gps object
TinyGPSPlus gps;

// software TX and RX pins for gps
int TXPin = 8;  // NodeMCU: GPIO14 corresponds to D5, UNO: D8
int RXPin = 9;  // NodeMCU: GPIO12 corresponds to D6, UNO: D9

SoftwareSerial gpsSerial(TXPin, RXPin);

// GPS values
double latCoordinate;
double longCoordinate;
double speed;
int hour;
int minute;
int second;

// accelerometer object
Adafruit_MPU6050 mpu;

// acelerometer values
double accelX;
double accelY;
double accelZ;
int accelRange;

/*******************************************************************************
 * Rotary Encoder 
 ******************************************************************************/
// Arduino and KY-040 module
int encoderCLK = 3; // CLK pin
int encoderDT = 2; // DT pin
int encoderSW = 0; // SW pin
int encoderCLK_prev;
int encoderCLK_value;
unsigned long last_run = 0;

/*******************************************************************************
 * EEPROM 
 ******************************************************************************/
#include <EEPROM.h>

/*******************************************************************************
*/

/*
#include "Accel_and_GPS.h"
#include "CAN_send.h"
#include "CAN_receive.h"
*/


//gauge angle matrix
int display_angle[3][4] = {
                          {0,0,0,0},
                          {0,0,0,0},
                          {0,0,0,0}
                          };

void setup() {
  Serial.begin(115200);

  // Init Display
  if (!gfx->begin()){
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

  for (int i = 0; i < 12; i++){
    Serial.print("Address: ");
    Serial.print(i);
    Serial.print("  Value: ");
    Serial.println(EEPROM.read(i));
  }
  for (int i = 0; i < 3; i++){ //iterate 0 -3 for profile number
    for (int j = 0; j < 4; j++){ //iterate 0 - 4 for gauge number
      display_angle[i][j] = EEPROM.read(i*4 + j);
    }
  }

  //CAN setup
  /*
  while (CAN_OK != CAN.begin(CAN_500KBPS)) {             // init can bus : baudrate = 500k
      // SERIAL_PORT_MONITOR.println("CAN init fail, retry...");
      Serial.println("CAN init fail, retry...");
      delay(100);
  }
  */
  // SERIAL_PORT_MONITOR.println("CAN init ok!");
  Serial.println("CAN init ok!");
  //CAN setup end

  //accelerometer setup
  while (!Serial)
    delay(10);  // will pause Zero, Leonardo, etc until serial console opens
  // Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    // Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  // Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
    case MPU6050_RANGE_2_G:
      // Serial.println("+-2G");
      break;
    case MPU6050_RANGE_4_G:
      // Serial.println("+-4G");
      break;
    case MPU6050_RANGE_8_G:
      // Serial.println("+-8G");
      break;
    case MPU6050_RANGE_16_G:
      // Serial.println("+-16G");
      break;
  }
  accelRange = mpu.getAccelerometerRange();

  // Serial.println("");
  // delay(100);
  Serial.flush();
  // Serial.end(); // not sure if this is needed
  //accelerometer setup end

  //GPS setup
  //gpsSerial.begin(9600);
  //GPS setup end
  printCSVHeaders();

  //encoder setup
  pinMode (encoderCLK, INPUT);
  pinMode (encoderDT, INPUT);
  pinMode(encoderSW, INPUT_PULLUP);
  encoderCLK_prev = digitalRead(encoderCLK);
  attachInterrupt(digitalPinToInterrupt(encoderCLK), shaft_moved, FALLING);
  attachInterrupt(digitalPinToInterrupt(encoderSW), buttonPressed, RISING);
}

//CAN RECEIVE
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
//CAN RECEIVE end


void accelerometerLoop() {
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // store the values
  accelX = a.acceleration.x;
  accelY = a.acceleration.y;
  accelZ = a.acceleration.z;

  Serial.flush();
}

void gpsLoop() {
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      // call function to print values in CSV format
      printCSVValues9600();
    }

    // If 5000 milliseconds pass and there are no characters coming in
    // over the software serial port, show a "No GPS detected" error
    if (millis() > 5000 && gps.charsProcessed() < 10) {
      Serial.begin(9600);
      Serial.println("No GPS detected");
      while (true) {}
    }
  }
}

void printCSVHeaders() {
  Serial.begin(9600);
  Serial.print('\n');
  Serial.println("Date,Time(UTC),Latitude,Longitude,Speed(km/h),Altitude(m),aX,aY,aZ,aRange(G)");
  Serial.flush();
  // Serial.end(); // not sure if this is needed
}

void printCSVValues9600() {
  Serial.begin(9600);

  // GPS values
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.print(gps.date.year());
    Serial.print(",");
  } else {
    Serial.print("DATENV");
    Serial.print(",");
  }

  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) {
      Serial.print(F("0"));
    }
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) {
      Serial.print(F("0"));
    }
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) {
      Serial.print(F("0"));
    }
    Serial.print(gps.time.second());
    Serial.print(".");
    if (gps.time.centisecond() < 10) {
      Serial.print(F("0"));
    }
    Serial.print(gps.time.centisecond());
    Serial.print(",");
  } else {
    Serial.print("TIMENV");
    Serial.print(",");
  }

  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(",");
    Serial.print(gps.location.lng(), 6);
    Serial.print(",");
  } else {
    // print for both lat and long columns
    Serial.print("LATNV");
    Serial.print(",");
    Serial.print("LONGNV");
    Serial.print(",");
  }

  if (gps.speed.isValid()) {
    Serial.print(gps.speed.kmph());
    Serial.print(",");
  } else {
    Serial.print("SPDNV");
    Serial.print(",");
  }

  if (gps.altitude.isValid()) {
    Serial.print(gps.altitude.meters());
    Serial.print(",");
  } else {
    Serial.print("ALTNV");
    Serial.print(",");
  }

  // accelerometer values
  Serial.print(accelX);
  Serial.print(",");
  Serial.print(accelY);
  Serial.print(",");
  Serial.print(accelZ);
  Serial.print(",");
  Serial.print(accelRange);
  Serial.print('\n');

  Serial.flush();
  // Serial.end(); // not sure if this is needed
}

//main loop variables
int8_t profile_num = 1; //1 to 3
int8_t arrow_pos = 1; //1 to 4
int8_t editAngleNum = 0; //1 to 5, 5 is save, 0 is off
boolean save = false;
boolean editAngle = false;

//interupt variables
volatile boolean interruptFlagArrow = false;
volatile boolean interruptFlagGaugeArrow = false;
volatile boolean interruptFlagGauge = false;
volatile boolean interruptFlagProfile = false;
volatile boolean interruptFlagEdit = false;


void loop(void) {
  accelerometerLoop(); //accelerometer data
  mainScreen();
  
  //CAN SEND
  /*
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
  */
  //CAN SEND end

  //CAN RECEIVE
  /*
  unsigned char len = 0;
  unsigned char rxBuf[8];
  int input = 5;

  if (CAN_MSGAVAIL == CAN.checkReceive()) {         // check if data coming
    CAN.readMsgBuf(&len, rxBuf);    // read data,  len: data length, buf: data buf
    unsigned long canId = CAN.getCanId();
    Serial.println("-----------------------------");
    Serial.println("get data from ID: 0x");
    if (len < 1) { return; }

  for(int i=3; i < len; i++) {
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
  //CAN RECEIVE end
  */
}

void shaft_moved(){
  int8_t pauseLen = 300;
  if (editAngle) { 
    pauseLen = 5;
  }
  if (millis()-last_run > pauseLen){
    if (digitalRead(encoderDT) == 0){ //CCW
      if (editAngle && display_angle[profile_num - 1][editAngleNum - 1] <= 92) {
        display_angle[profile_num - 1][editAngleNum - 1] += 8;
        interruptFlagGauge = true;
      } else if (editAngleNum && !editAngle) {
        if (editAngleNum == 5) {
          editAngleNum = 1;
        } else {
          editAngleNum++;
        }
        interruptFlagGaugeArrow = true;
      } else if (!editAngleNum && !editAngle) {
        if (arrow_pos == 4) {
          arrow_pos = 1;
        } else {
          arrow_pos++;
        }
        interruptFlagArrow = true;
      }
    }
    if (digitalRead(encoderDT) == 1){ //CW
      if (editAngle && display_angle[profile_num - 1][editAngleNum - 1] >= 8) {
        display_angle[profile_num - 1][editAngleNum - 1] -= 8;
        interruptFlagGauge = true;
      } else if (editAngleNum && !editAngle) { 
        if (editAngleNum == 1) {
          editAngleNum = 5;
        } else {
          editAngleNum--;
        }
        interruptFlagGaugeArrow = true;
      } else if (!editAngleNum && !editAngle) {
        if (arrow_pos == 1) {
          arrow_pos = 4;
        } else {
          arrow_pos--;
        }
        interruptFlagArrow = true;
      }
    }
    last_run=millis();
  }
}

void buttonPressed() {
  if (millis()-last_run > 100){
    Serial.println("Button Pressed");
    if (arrow_pos <= 3) { //profile selection
      profile_num = arrow_pos;
      interruptFlagProfile = true;
      interruptFlagGauge = true;
      //call to CAN function to change the angles if they are different
    } else if (arrow_pos == 4 && editAngleNum == 0){ //turning on edit mode
      save = true;
      editAngleNum = 1;
      interruptFlagEdit = true;
      interruptFlagGaugeArrow = true;
    } else if ((editAngleNum >= 1 && editAngleNum <= 4) && editAngle == false) { //turning on edit angle mode
      editAngle = true;
    } else if (editAngle == true && editAngleNum <= 4){ //turning off edit angle mode
      editAngle = false;
    } else if (editAngleNum == 5) { //turning off edit mode
      save = false;
      editAngleNum = 0;
      interruptFlagEdit = true;
      //call to CAN function to change the angles if they are different

      //EEPROM write to commit data
      for (int i = 0; i < 3; i++){ //iterate 0 -3 for profile number
        for (int j = 0; j < 4; j++){ //iterate 0 - 4 for gauge number
          EEPROM.write(i * 4 + j, display_angle[i][j]);
        }
      }
    }
    last_run=millis();
  }
}

void mainScreen() {
  //gfx->setFont(u8g2_font_Pixellari_tu);
  gfx->setTextColor(gfx->color565(0xff, 0xff, 0xff));

  gfx->setTextSize(4);
  gfx->setCursor(260, 150);
  gfx->print(F("PROFILES"));
  gfx->setTextSize(2);
  gfx->setCursor(290, 200);
  gfx->print(F("PROFILE 1 "));
  gfx->setCursor(290, 230);
  gfx->print(F("PROFILE 2 "));
  gfx->setCursor(290, 260);
  gfx->print(F("PROFILE 3 "));

  if (interruptFlagEdit) {
    gfx->fillRect(290, 290, 50, 22, BLACK); //edit-save
    interruptFlagEdit = false;
  }
  gfx->setCursor(290, 290);
  if (!save) {
    gfx->print(F("EDIT"));
  } else {
    gfx->setTextColor(gfx->color565(0xe4, 0x2b, 0x37));
    gfx->print(F("SAVE"));
  }

  gfx->setTextColor(gfx->color565(0xff, 0xff, 0xff));
  gfx->setCursor(420, 40);
  gfx->print(F("MPH"));
  gfx->setCursor(420, 100);
  gfx->print(F("m/s"));

  if (interruptFlagProfile) {
    gfx->fillRect(156, 280, 20, 20, BLACK); //profile
    interruptFlagProfile = false;
  }
  gfx->setCursor(60, 280);
  gfx->print(F("PROFILE "));
  gfx->println(profile_num);

  gfx->setTextSize(1);
  gfx->setCursor(30, 80);
  gfx->print(display_angle[profile_num - 1][0]);
  gfx->println(F("%"));
  gfx->setCursor(174, 80);
  gfx->print(display_angle[profile_num - 1][1]);
  gfx->println(F("%"));
  gfx->setCursor(30, 220);
  gfx->print(display_angle[profile_num - 1][2]);
  gfx->println(F("%"));
  gfx->setCursor(174, 220);
  gfx->print(display_angle[profile_num - 1][3]);
  gfx->println(F("%"));

  if (editAngle){
    gfx->setTextColor(gfx->color565(0xe4, 0x2b, 0x37));
  }
  if (interruptFlagGaugeArrow) {
    gfx->fillRect(80, 66, 64, 170, BLACK); //edit gauge arrows
    gfx->fillRect(260, 280, 22, 26, BLACK); //edit-save arrow
    interruptFlagGaugeArrow = false;
  }
  gfx->setTextSize(3);
  if (editAngleNum == 1 || editAngleNum == 3){
    gfx->setCursor(80, 66  + (editAngleNum - 1) * 70);
    gfx->print(F("<-"));
  } else if (editAngleNum == 2 || editAngleNum == 4) {
    gfx->setCursor(110, 66  + (editAngleNum - 2) * 70);
    gfx->print(F("->"));
  } else if (editAngleNum == 5) {
    gfx->setTextSize(2);
    gfx->setCursor(260, (200 + 30 * (arrow_pos - 1)));
    gfx->print(F("->"));
  }

  gfx->setTextColor(gfx->color565(0xe4, 0x2b, 0x37));
  gfx->setTextSize(5);
  gfx->fillRect(260, 20, 150, 40, BLACK);
  gfx->setCursor(260, 20);
  gfx->print(speed, 2);
  gfx->fillRect(260, 80, 150, 40, BLACK);
  gfx->setCursor(260, 80);
  gfx->print(-1*accelY, 2);

  if (!save){
    if (interruptFlagArrow) {
      if (arrow_pos == 1) {
        gfx->fillRect(260, 220, 22, 100, BLACK);
      }
      else if (arrow_pos == 2) {
        gfx->fillRect(260, 200, 22, 26, BLACK);
        gfx->fillRect(260, 250, 22, 80, BLACK);
      } 
      else if (arrow_pos == 3) {
        gfx->fillRect(260, 200, 22, 50, BLACK);
        gfx->fillRect(260, 280, 22, 26, BLACK);
      }
      else if (arrow_pos == 4) {
        gfx->fillRect(260, 200, 22, 80, BLACK);
      }
      interruptFlagArrow = false;
    }
    gfx->setTextSize(2);
    gfx->setCursor(260, (200 + 30 * (arrow_pos - 1)));
    gfx->print(F("->"));
  }

  if (interruptFlagGauge) {
    if (editAngleNum == 1){
      gfx->fillRect(6, 40, 68, 60, BLACK);
    } else if (editAngleNum == 2) {
      gfx->fillRect(150, 40, 68, 60, BLACK);
    } else if (editAngleNum == 3) {
      gfx->fillRect(6, 180, 68, 60, BLACK);
    } else if (editAngleNum == 4) {
      gfx->fillRect(150, 180, 68, 60, BLACK);
    } else { //switching profile clear all
      gfx->fillRect(6, 40, 68, 60, BLACK);
      gfx->fillRect(150, 40, 68, 60, BLACK);
      gfx->fillRect(6, 180, 68, 60, BLACK);
      gfx->fillRect(150, 180, 68, 60, BLACK);
    }
    interruptFlagGauge = false;
  }
  //390 end - 150 start = 240
  //240 div by 30 = 8 degree sections
  //fillArc       ( x, y, r0, r1, angle0, angle1, color);
  gfx->fillArc(40, 80, 33, 30, 150, 150 + (display_angle[profile_num - 1][0] / 100.00) * 240, gfx->color565(0xe4, 0x2b, 0x37));
  gfx->fillArc(184, 80, 33, 30, 150, 150 + (display_angle[profile_num - 1][1] / 100.00) * 240, gfx->color565(0xe4, 0x2b, 0x37));
  gfx->fillArc(40, 220, 33, 30, 150, 150 + (display_angle[profile_num - 1][2] / 100.00) * 240, gfx->color565(0xe4, 0x2b, 0x37));
  gfx->fillArc(184, 220, 33, 30, 150, 150 + (display_angle[profile_num - 1][3] / 100.00) * 240, gfx->color565(0xe4, 0x2b, 0x37));
}
