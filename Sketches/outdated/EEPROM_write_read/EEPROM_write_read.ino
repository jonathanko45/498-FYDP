#include <EEPROM.h>

// 8 kB of EEPROM
// each byte can hold a value between 0-255
int addr = 0;
int read_value;
int random_number;

void setup() {
  Serial.begin(9600);
  delay(15000);



  // Serial.print("Address 0: ");
  // Serial.println(temp);

  // // write
  // EEPROM.write(addr, random_number);
}

void loop() {
  // create random number from 0 to 255
  randomSeed(analogRead(0));
  random_number = random(0, 256);

  Serial.print("random_number is: ");
  Serial.println(random_number);

  // read
  read_value = EEPROM.read(addr);
  Serial.print("Address 0 contains: ");
  Serial.println(read_value);

  // write
  Serial.print("Now writing ");
  Serial.print(random_number);
  Serial.println(" to Address 0");
  EEPROM.write(addr, random_number);

  Serial.println();

  delay(10000);
}
