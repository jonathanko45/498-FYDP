#include <TinyGPS++.h>

int GPSBaud = 9600;

// TinyGPS++ object
TinyGPSPlus gps;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  printHeaders();

}

void loop() {
  // put your main code here, to run repeatedly:

  while(Serial.available()) {

    if (gps.encode(Serial.read())) {
    
      printForCSV();

    }

  }

}

void GPSSetup() {

  // Serial.begin(9600);
  while (!Serial) {
    delay(10); // will pause program until serial console opens
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

void printForCSV() {
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
    Serial.print(",");
    // Serial.print('\n');
  }

  // accelerometer data
  
}