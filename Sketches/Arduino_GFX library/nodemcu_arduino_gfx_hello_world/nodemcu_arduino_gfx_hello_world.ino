/*******************************************************************************
 * Modified from HelloWorld example of Arduino_GFX
 * run on Nodemcu esp8266 + 480x320 ILI9488 SPI TFT
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

#define TFT_CS 16    //GPIO16 - D0
#define TFT_RESET 5  //GPIO5 - D1
#define TFT_DC 4     //GPIO4 - D2
#define TFT_MOSI 13  //GPIO13/MOSI - D7
#define TFT_SCK 14   //GPIO14/SCK - D5
#define TFT_LED 0    //GPIO0 - D3
#define TFT_MISO -1  // not used for TFT

#define GFX_BL TFT_LED  // backlight pin

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, TFT_RESET, 3 /* rotation */, false /* IPS */);


/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

void setup(void) {
  gfx->begin();
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  gfx->setCursor(10, 10);
  gfx->setTextColor(RED);
  gfx->println("Hello World!");

  delay(5000);  // 5 seconds
}

void loop() {
  gfx->setCursor(random(gfx->width()), random(gfx->height()));
  gfx->setTextColor(random(0xffff), random(0xffff));
  gfx->setTextSize(random(6) /* x scale */, random(6) /* y scale */, random(2) /* pixel_margin */);
  gfx->println("Hello World!");

  delay(1000);  // 1 second
}