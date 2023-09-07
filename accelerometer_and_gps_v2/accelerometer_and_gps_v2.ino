// This sketch was created in hopes of solving the serial communication freezing issue

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
int TXPin = 14;  // NodeMCU: GPIO14 corresponds to D5, UNO: 2
int RXPin = 12;  // NodeMCU: GPIO12 corresponds to D6, UNO: 3

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
// SCL:
// SDA: 

// acelerometer values
double accelX;
double accelY;
double accelZ;
int accelRange;

void setup() {
  accelerometerSetup();

  gpsSetup();

  printCSVHeaders();
}

void loop() {
  checkSerialConnection();

  // config 1
  accelerometerLoop();
  gpsLoop();

  // config 2
  // gpsLoop2();

  delay(100);
}

void accelerometerSetup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);  // will pause Zero, Leonardo, etc until serial console opens

  // Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
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

void gpsLoop2() {

  if (gps.encode(gpsSerial.read())) {
    getGPSData();
  } else {
    hour = 88;
    minute = 88;
    second = 88;
    latCoordinate = 350;
    longCoordinate = 350;
    speed = 1111;
  }

  printCSVValues115200();
}

void printCSVHeaders() {
  Serial.begin(9600);
  Serial.print('\n');
  Serial.println("Date,Time(UTC),Latitude,Longitude,Speed(km/h),Altitude(m),aX,aY,aZ,aRange(G)");
  Serial.flush();
  // Serial.end(); // not sure if this is needed
}

void printCSVHeaders2() {
  Serial.begin(115200);
  Serial.print('\n');
  Serial.println("Time(UTC),Latitude,Longitude,Speed(km/h),aX,aY,aZ");
  Serial.flush();
}

void getGPSData() {
  Serial.begin(9600);

  if (gps.time.isValid()) {
    hour = gps.time.hour();
    minute = gps.time.minute();
    second = gps.time.second();
  } else {
    hour = 99;
    minute = 99;
    second = 99;
  }

  if (gps.location.isValid()) {
    latCoordinate = gps.location.lat();
    longCoordinate = gps.location.lng();
  } else {
    latCoordinate = 360;
    longCoordinate = 360;
  }

  if (gps.speed.isValid()) {
    speed = gps.speed.kmph();
  } else {
    speed = 1234;
  }

  Serial.flush();
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

void printCSVValues115200() {

  Serial.begin(115200);

  if (hour < 10) {
    Serial.print(F("0"));
  }
  Serial.print(hour);
  Serial.print(":");
  if (minute < 10) {
    Serial.print(F("0"));
  }
  Serial.print(minute);
  Serial.print(":");
  if (second < 10) {
    Serial.print(F("0"));
  }
  Serial.print(second);
  Serial.print(",");

  Serial.print(latCoordinate);
  Serial.print(",");
  Serial.print(longCoordinate);
  Serial.print(",");

  Serial.print(speed);
  Serial.print(",");

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  Serial.print(a.acceleration.x);
  Serial.print(",");
  Serial.print(a.acceleration.y);
  Serial.print(",");
  Serial.print(a.acceleration.z);
  Serial.print('\n');

  Serial.flush();
}

void checkSerialConnection() {
  if (!Serial) {   //check if Serial is available... if not,
    Serial.end();  // close serial port
    delay(100);    //wait 100 millis
    // Serial.begin(9600);  // reenable serial again
  }
}