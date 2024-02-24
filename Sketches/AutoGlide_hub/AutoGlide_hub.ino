#include <EEPROM.h>
#include "Accel_and_GPS.h"
#include "CANSendReceive_hub.h"

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
int display_angle[4][4] = {
                          {10,10,10,10},
                          {50,50,50,50},
                          {90,90,90,90},
                          {10,10,10,10}
                          };


//main loop variables
int8_t profile_num = 1; //1 to 4
int8_t arrow_pos = 1; //1 to 5
int8_t editAngleNum = 0; //1 to 5, 5 is save, 0 is off
boolean save = false;
boolean editAngle = false;
volatile int graphLoop = 0;
float graphY = 0;
float Y_val = 0;
float avgAccel[34];
float upperLim = 25;
int strike = 0;


//interupt variables
volatile boolean interruptFlagArrow = false;
volatile boolean interruptFlagGaugeArrow = false;
volatile boolean interruptFlagGauge = false;
volatile boolean interruptFlagProfile = false;
volatile boolean interruptFlagEdit = false;
volatile boolean interruptFlagUpdate = false;
volatile boolean interruptFlagAuto = false;
volatile boolean interruptFlagAutoRefresh = false;

void setup() {
  Serial.begin(115200);
  //while(!Serial);

  //Graphics setup
  if (!gfx->begin()){
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

  accelerometerSetup();
  gpsSetup();
  printCSVHeaders();

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
  for (int i = 0; i < 13; i++){
    Serial.print("Address: ");
    Serial.print(i);
    Serial.print("  Value: ");
    Serial.println(EEPROM.read(i));
  }*/
  /*
  for (int i = 0; i < 4; i++){ //iterate 0 - 3 for profile number
    for (int j = 0; j < 4; j++){ //iterate 0 - 3 for gauge number
      display_angle[i][j] = EEPROM.read(i * 4 + j);
    }
  }*/
}

void loop(void) {
  if (interruptFlagUpdate){
    Serial.println("Changing stiffness...");
    update_motor();
    interruptFlagUpdate = false;
  }
 
  accelerometerLoop(); //accelerometer data
  gpsLoop();

  mainScreen();

  if (interruptFlagAuto){
    if (graphLoop == 34){
      autoProfile();
      graphLoop = 0;
    } else {
      graphLoop++;
      graph();
    }
  } 
  delay(100); //refresh rate
}


void update_motor() {
  updateScreen();
  for (int i = 0; i < 4; i++){
    turn_done[i] = false;
  }
  if (profile_num == 1) {// send address 0-3 to motor board 1 through 4
    for (int i = 0; i < 4; i++) {
      SendStiffness(display_angle[profile_num - 1][i] / 10, i + 1);
      Serial.println(display_angle[profile_num - 1][i] / 10);
    }
  } else if (profile_num == 2) {
    for (int i = 4; i < 8; i++) { // send address 4-7 to motor board 1 through 4
      SendStiffness(display_angle[profile_num - 1][i - 4] / 10, i - 3);
      Serial.println(display_angle[profile_num - 1][i - 4] / 10);
    }
  } else if (profile_num == 3) { // send address 8-11 to motor board 1 through 4
    for (int i = 8; i < 12; i++) {
      SendStiffness(display_angle[profile_num - 1][i - 8] / 10, i - 7);
      Serial.println(display_angle[profile_num - 1][i - 8] / 10);
    }
  }
  allMotorsDone();
  gfx->fillScreen(BLACK);
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
      } else if (!editAngleNum && !editAngle && !interruptFlagAuto) {
        if (arrow_pos == 5) {
          arrow_pos = 1;
        } else {
          arrow_pos++;
        }
        interruptFlagArrow = true;
      } else if (!editAngleNum && !editAngle ) {
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
      } else if (!editAngleNum && !editAngle && !interruptFlagAuto) {
        if (arrow_pos == 1) {
          arrow_pos = 5;
        } else {
          arrow_pos--;
        }
        interruptFlagArrow = true;
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
    if (arrow_pos <= 4) { //profile selection
      if (arrow_pos == 4 && !interruptFlagAuto){
        interruptFlagAuto = true;
        graphLoop = 0;
        interruptFlagAutoRefresh = true;
        profile_num = arrow_pos;
      } else if (arrow_pos == 4 && interruptFlagAuto) {
        interruptFlagAuto = false;
        interruptFlagAutoRefresh = true;
        profile_num = arrow_pos;
      } else {
        if (profile_num == 4) {
          interruptFlagAutoRefresh = true;
        }
        profile_num = arrow_pos;
        //EEPROM.write(12, profile_num);
        interruptFlagProfile = true;
        interruptFlagAuto = false;

        //call to CAN function to change the angles if they are different
        interruptFlagUpdate = true;
      }
      interruptFlagGauge = true;
    } else if (arrow_pos == 5 && editAngleNum == 0){ //turning on edit mode
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
      /*
      for (int i = 0; i < 4; i++){ //iterate 0 - 3 for profile number
        for (int j = 0; j < 4; j++){ //iterate 0 - 3 for gauge number
          EEPROM.write(i * 4 + j, display_angle[i][j]);
        }
      }*/

      //call to CAN function to change the angles if they are different
      interruptFlagUpdate = true;
    }
    last_run=millis();
  }
}

void graph(){
  gfx->setTextSize(1);
  gfx->setTextColor(gfx->color565(0xff, 0xff, 0xff));
  gfx->fillRect(260, 20, 2, 110, WHITE); //y-axis line
  gfx->fillRect(262, 50, 204, 1, gfx->color565(0x80, 0x80, 0x80)); //upper lim
  
  gfx->fillRect(262, 75, 204, 1, gfx->color565(0x80, 0x80, 0x80)); //middle x-axis
  gfx->fillRect(256, 75, 6, 2, gfx->color565(0xff, 0xff, 0xff));
  gfx->setCursor(236, 70);
  gfx->print(F("9.8"));

  gfx->fillRect(262, 100, 204, 1, gfx->color565(0x80, 0x80, 0x80)); //lower lim
 
  graphY = 2.3 * sq(abs(-1*accelY - 9.8)); //multiplied by ratio 2.3 for sensitivity
  avgAccel[graphLoop] = graphY;

  if (graphY >= 55){
    Y_val = 55;
  } else {
    Y_val = graphY;
  }
  if (accelY + 9.8 >= 0) {
    gfx->fillRect(258 + 6*graphLoop, (75 + Y_val - 2), 4, 4, gfx->color565(0xe4, 0x2b, 0x37));
  } else if (accelY + 9.8 < 0){
    gfx->fillRect(268 + 6*graphLoop, (75 - Y_val), 4, 4, gfx->color565(0xe4, 0x2b, 0x37));
  }
}

void autoProfile(){
  //if difference between each value in last ~34 sec is large (greater than X strikes) -> soften
  //if difference between each value in last ~34 sec is small (fewer than Y strikes) -> stiffen

  for (int i = 0; i < 35; i++){
    if (avgAccel[i] >= upperLim){
      strike++;
    } 
  }
  if (strike >= 5) {
    if (display_angle[3][0] > 50 ||  display_angle[3][1] > 50 || display_angle[3][2] > 50 || display_angle[3][3] > 50) { //soften by 1 level
      display_angle[3][0] = 50;
      display_angle[3][1] = 50;
      display_angle[3][2] = 50;
      display_angle[3][3] = 50;
      interruptFlagUpdate = true;
    } else if (display_angle[3][0] > 10 ||  display_angle[3][1] > 10 || display_angle[3][2] > 10 || display_angle[3][3] > 10){ //soften by 2 levels
      display_angle[3][0] = 10;
      display_angle[3][1] = 10;
      display_angle[3][2] = 10;
      display_angle[3][3] = 10;
      interruptFlagUpdate = true;
    }
    
  } else if (strike <= 2) {
    if (display_angle[3][0] < 50 ||  display_angle[3][1] < 50 || display_angle[3][2] < 50 || display_angle[3][3] < 50) { //tighten by 1 level
      display_angle[3][0] = 50;
      display_angle[3][1] = 50;
      display_angle[3][2] = 50;
      display_angle[3][3] = 50;
      interruptFlagUpdate = true;
    } else if (display_angle[3][0] < 90 ||  display_angle[3][1] < 90 || display_angle[3][2] < 90 || display_angle[3][3] < 90) { //tighten by 2 levels
      display_angle[3][0] = 90;
      display_angle[3][1] = 90;
      display_angle[3][2] = 90;
      display_angle[3][3] = 90;
      interruptFlagUpdate = true;
    }
  }
  strike = 0;
  gfx->fillRect(262, 20, 280, 116, BLACK);
}

void mainScreen() {
  gfx->setTextColor(gfx->color565(0xff, 0xff, 0xff));
  gfx->setTextSize(3);
  gfx->setCursor(260, 150);
  gfx->print(F("PROFILES"));
  gfx->setTextSize(2);
  gfx->setCursor(290, 186);
  gfx->print(F("PROFILE 1"));
  gfx->setCursor(290, 212);
  gfx->print(F("PROFILE 2"));
  gfx->setCursor(290, 238);
  gfx->print(F("PROFILE 3"));
  gfx->setCursor(290, 264);
  if (interruptFlagAuto){
    gfx->setTextColor(gfx->color565(0xe4, 0x2b, 0x37));
  } 
  gfx->print(F("AUTO"));
  gfx->setTextColor(gfx->color565(0xff, 0xff, 0xff));

  if (interruptFlagAutoRefresh) {
    gfx->fillRect(230, 10, 290, 180, BLACK); //for graph
    gfx->fillRect(30, 280, 200, 30, BLACK); //for profile 
    interruptFlagAutoRefresh = false;
  }

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

  if (!interruptFlagAuto){
    gfx->setTextColor(gfx->color565(0xff, 0xff, 0xff));
    gfx->setCursor(420, 40);
    gfx->print(F("KPH"));
    gfx->setCursor(420, 100); //labels
    gfx->print(F("m/s"));
    gfx->setCursor(460, 94);
    gfx->setTextSize(1);
    gfx->print(F("2"));

    gfx->setTextColor(gfx->color565(0xe4, 0x2b, 0x37)); //values
    gfx->setTextSize(5);
    gfx->fillRect(260, 80, 150, 40, BLACK);
    gfx->setCursor(260, 80);
    gfx->print(-1*accelY, 2);

    gfx->fillRect(260, 20, 150, 40, BLACK);
    gfx->setCursor(260, 20);
    gfx->print(speed, 1);
  }

  if (interruptFlagProfile) {
    gfx->fillRect(170, 280, 24, 24, BLACK); //profile
    interruptFlagProfile = false;
  }
  gfx->setTextColor(gfx->color565(0xff, 0xff, 0xff));
  gfx->setTextSize(3); 
  if (profile_num <= 3){
    gfx->setCursor(30, 280);
    gfx->print(F("PROFILE "));
    gfx->println(profile_num);
  } else if (profile_num == 4){
    gfx->setCursor(64, 280);
    gfx->print(F("AUTO"));
  }

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
    gfx->setCursor(260, (186 + 26 * (arrow_pos - 1)));
    gfx->print(F("->"));
  }

  if (!save){
    if (interruptFlagArrow) {
      if (arrow_pos == 1) {
        gfx->fillRect(260, 200, 22, 110, BLACK);
      }
      else if (arrow_pos == 2) {
        gfx->fillRect(260, 180, 22, 24, BLACK);
        gfx->fillRect(260, 230, 22, 80, BLACK);
      } 
      else if (arrow_pos == 3) {
        gfx->fillRect(260, 180, 22, 50, BLACK);
        gfx->fillRect(260, 260, 22, 50, BLACK);
      }
      else if (arrow_pos == 4) {
        gfx->fillRect(260, 180, 22, 80, BLACK);
        gfx->fillRect(260, 280, 22, 30, BLACK);
      }
      else if (arrow_pos == 5) {
        gfx->fillRect(260, 180, 22, 100, BLACK);
      }
      interruptFlagArrow = false;
    }
    gfx->setTextSize(2);
    gfx->setTextColor(gfx->color565(0xe4, 0x2b, 0x37));
    gfx->setCursor(260, (186 + 26 * (arrow_pos - 1)));
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

  gfx->fillArc(40, 80, 33, 30, 150, 150 + (display_angle[profile_num - 1][0] / 100.00) * 240, gfx->color565(0xe4, 0x2b, 0x37));
  gfx->fillArc(184, 80, 33, 30, 150, 150 + (display_angle[profile_num - 1][1] / 100.00) * 240, gfx->color565(0xe4, 0x2b, 0x37));
  gfx->fillArc(40, 220, 33, 30, 150, 150 + (display_angle[profile_num - 1][2] / 100.00) * 240, gfx->color565(0xe4, 0x2b, 0x37));
  gfx->fillArc(184, 220, 33, 30, 150, 150 + (display_angle[profile_num - 1][3] / 100.00) * 240, gfx->color565(0xe4, 0x2b, 0x37));
}

void updateScreen(){
  gfx->fillRect(100, 100, 290, 120, BLUE);
  gfx->setTextColor(gfx->color565(0xff, 0xff, 0xff));
  gfx->setTextSize(4);
  gfx->setCursor(150, 150); 
  gfx->print(F("UPDATING"));
}
