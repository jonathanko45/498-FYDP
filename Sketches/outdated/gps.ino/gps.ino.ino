// SoftwareSerial library allows serial communication on other digital pins on the Arduino Uno
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// NMEA reference manual: https://www.sparkfun.com/datasheets/GPS/NMEA%20Reference%20Manual-Rev2.1-Dec07.pdf

// TX and RX pins to NodeMCU (or Arduino Uno)
int TXPin = 2; // GPIO14 corresponds to D5 (Uno: 2)
int RXPin = 3; // GPIO12 corresponds to D6 (Uno: 3)

// baudrate of NEO-6M
int GPSBaud = 9600;


// software serial port called "gpsSerial"
SoftwareSerial gpsSerial(TXPin, RXPin);

// TinyGPS++ object
TinyGPSPlus gps;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  // Serial.begin(115200);

  // start software serial port
  gpsSerial.begin(GPSBaud);
  //            Serial.begin(GPSBaud);

  // print headers for CSV file
  printHeaders();
}

// put your main code here, to run repeatedly:
void loop() {

  while (gpsSerial.available() > 0) {
    // while (Serial.available()) {

    //Serial.println("in here2");

    if (gps.encode(gpsSerial.read())) {
      // if (gps.encode(Serial.read())) {
      // displayInfo();
      printToCSV();
    }

    // If 5000 milliseconds pass and there are no characters coming in
    // over the software serial port, show a "No GPS detected" error
    if (millis() > 5000 && gps.charsProcessed() < 10) {
      Serial.println("No GPS detected");
      while (true) {}
    }
  }
}

void displayInfo() {
  if (gps.location.isValid()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());
    Serial.print("Speed(km/h): ");
    Serial.println(gps.speed.kmph());
  } else {
    Serial.println("Location: Not Available");
  }

  Serial.print("Date: ");
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.println(gps.date.year());
  } else {
    Serial.println("Not Available");
  }

  Serial.print("Time(UTC): ");
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(".");
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.println(gps.time.centisecond());
  } else {
    Serial.println("Not Available");
  }

  Serial.println();
  Serial.println();
  delay(1000);
}

void printToCSV() {
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.print(gps.date.year());
    Serial.print(",");
  } else {
    Serial.print("date not valid");
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
    Serial.print("time not valid");
    Serial.print(",");
  }

  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(",");
    Serial.print(gps.location.lng(), 6);
    Serial.print(",");
  } else {
    // print for both lat and long columns
    Serial.print("location not valid");
    Serial.print(",");
    Serial.print("location not valid");
    Serial.print(",");
  }

  if (gps.speed.isValid()) {
    Serial.print(gps.speed.kmph());
    Serial.print(",");
  } else {
    Serial.print("speed not valid");
    Serial.print(",");
  }

  if (gps.altitude.isValid()) {
    Serial.print(gps.altitude.meters());
    Serial.print('\n');
  } else {
    Serial.print("altitude not valid");
    Serial.print('\n');
  }
}

void printHeaders() {
  Serial.print('\n');
  Serial.print("Date");
  Serial.print(",");
  Serial.print("Time(UTC)");
  Serial.print(",");
  Serial.print("Latitude");
  Serial.print(",");
  Serial.print("Longitude");
  Serial.print(",");
  Serial.print("Speed(km/h)");
  Serial.print(",");
  Serial.print("Altitude(m)");
  Serial.print('\n');
}
