/*
  LiquidCrystal Library - Hello World

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe
 modified 7 Nov 2016
 by Arturo Guadalupi

 This example code is in the public domain.

 https://docs.arduino.cc/learn/electronics/lcd-displays

*/

// LCD library
#include <LiquidCrystal.h>

// initialize the LCD library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

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
int TXPin = 8;  // NodeMCU: GPIO14 corresponds to D5, UNO: 8
int RXPin = 9;  // NodeMCU: GPIO12 corresponds to D6, UNO: 9

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

// I2C on NodeMCU:
// SCL: GPIO5 corresponds to D1
// SDA: GPIO4 corresponds to D2

// I2C on Uno:
// SCL: D19
// SDA: D18

// acelerometer values
double accelX;
double accelY;
double accelZ;
int accelRange;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  // lcd.print("hello, world!");
  
  accelerometerSetup();

  gpsSetup();

  printCSVHeaders();
}

void loop() {
  checkSerialConnection();

  accelerometerLoop();
  gpsLoop();

  delay(100);
}

void accelerometerSetup() {
  Serial.begin(115200);
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
  // Serial.println("MPU6050 Found!");

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
}

void gpsSetup() {
  gpsSerial.begin(9600);
}

void accelerometerLoop() {
  Serial.begin(115200);
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* Print out the values 
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");

  Serial.println("");
  */

  // store the values
  accelX = a.acceleration.x;
  accelY = a.acceleration.y;
  accelZ = a.acceleration.z;

  // delay(500);
  Serial.flush();
  // Serial.end(); // not sure if this is needed
}

void gpsLoop() {

  while (gpsSerial.available() > 0) {

    if (gps.encode(gpsSerial.read())) {
      // call function to print values in CSV format
      printCSVValues9600();

      // call function to print Lat and Long coordinates to LCD display (for debugging)
      printLatLongLCD();
    }

    // If 5000 milliseconds pass and there are no characters coming in
    // over the software serial port, show a "No GPS detected" error
    if (millis() > 5000 && gps.charsProcessed() < 10) {
      Serial.begin(9600);
      Serial.println("No GPS detected");
      while (true) {
      }
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

void checkSerialConnection() {
  if (!Serial) {   //check if Serial is available... if not,
    Serial.end();  // close serial port
    delay(100);    //wait 100 millis
    // Serial.begin(9600);  // reenable serial again
  }
}

void printLatLongLCD() {
  lcd.setCursor(0, 0);
  lcd.print(gps.location.lat(),6);
  
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  lcd.print(gps.location.lng(),6);
}