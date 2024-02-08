#include <EEPROM.h>
#include "Accel_and_GPS.h"
#include "CANSendReceive_hub.h"

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

// Arduino and KY-040 module
int encoderCLK = 3; // CLK pin
int encoderDT = 2; // DT pin
int encoderSW = 0; // SW pin
int encoderCLK_prev;
int encoderCLK_value;
unsigned long last_run = 0;


//gauge angle matrix
int display_angle[3][4] = {
                          {0,0,0,0},
                          {0,0,0,0},
                          {0,0,0,0}
                          };


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

void setup() {
  //Graphics setup
  if (!gfx->begin()){
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

  //serialSetup();
  accelerometerSetup();
  //gpsSetup();
  //printCSVHeaders();

  CANsetup();

  //encoder setup
  pinMode (encoderCLK, INPUT);
  pinMode (encoderDT, INPUT);
  pinMode(encoderSW, INPUT_PULLUP);
  encoderCLK_prev = digitalRead(encoderCLK);
  attachInterrupt(digitalPinToInterrupt(encoderCLK), shaft_moved, FALLING);
  attachInterrupt(digitalPinToInterrupt(encoderSW), buttonPressed, RISING);

  //EEPROM setup
  /*
  for (int i = 0; i < 12; i++){
    Serial.print("Address: ");
    Serial.print(i);
    Serial.print("  Value: ");
    Serial.println(EEPROM.read(i));
    //EEPROM.write(i, 0);
  }*/
  
  for (int i = 0; i < 4; i++){ //iterate 0 -3 for profile number
    for (int j = 0; j < 5; j++){ //iterate 0 - 4 for gauge number
      display_angle[i][j] = EEPROM.read(i*4 + j);
    }
  }
  profile_num = EEPROM.read(12);

  //send CAN message to check real values
  update_motor();
}

void loop(void) {
  accelerometerLoop(); //accelerometer data
  //gpsLoop();
  mainScreen();

  delay(100);
}

void serialSetup(){
  Serial.begin(115200);
}

void update_motor() {
  if (profile_num == 1) {// send address 0-3 to motor board 1 through 4
    for (int i = 0; i < 4; i++) {
      SendStiffness(display_angle[profile_num - 1][i] / 10, i + 1);
    }
  } else if (profile_num == 2) {
    for (int i = 4; i < 8; i++) { // send address 4-7 to motor board 1 through 4
      SendStiffness(display_angle[profile_num - 1][i] / 10, i - 3);
    }
  } else if (profile_num == 3) { // send address 8-11 to motor board 1 through 4
    for (int i = 8; i < 12; i++) { // send address 4-7 to motor board 1 through 4
      SendStiffness(display_angle[profile_num - 1][i] / 10, i - 7);
    }
  }
}

void shaft_moved(){
  int8_t pauseLen = 300;
  if (editAngle) { 
    pauseLen = 5;
  }
  if (millis()-last_run > pauseLen){
    if (digitalRead(encoderDT) == 0){ //CCW
      if (editAngle && display_angle[profile_num - 1][editAngleNum - 1] <= 90) {
        display_angle[profile_num - 1][editAngleNum - 1] += 10;
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
      if (editAngle && display_angle[profile_num - 1][editAngleNum - 1] >= 10) {
        display_angle[profile_num - 1][editAngleNum - 1] -= 10;
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
      EEPROM.write(12, profile_num);
      interruptFlagProfile = true;
      interruptFlagGauge = true;

      //call to CAN function to change the angles if they are different
      updateScreen();
      update_motor();
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

      //EEPROM write to commit data
      for (int i = 0; i < 4; i++){ //iterate 0 -3 for profile number
        for (int j = 0; j < 5; j++){ //iterate 0 - 4 for gauge number
          EEPROM.write(i * 4 + j, display_angle[i][j]);
        }
      }

      //call to CAN function to change the angles if they are different
      updateScreen();
      update_motor();
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

void updateScreen(){
  gfx->fillRect(150, 100, 200, 80, BLUE);

  gfx->setTextColor(gfx->color565(0xe4, 0x2b, 0x37));
  gfx->setTextSize(2);
  gfx->setCursor(200, 130);
  gfx->print(F("UPDATING STIFFNESS..."));

  //delay (10000);
  gfx->fillScreen(BLACK);
}
