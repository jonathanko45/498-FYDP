/*******************************************************************************
 * Modified from HelloWorld example of Arduino_GFX
 * run on Minima + 480x320 ILI9488 SPI TFT
 * PRINTF statements not working
 ******************************************************************************/
#include <Arduino_GFX_Library.h>

#define TFT_CS 10    //D10 or SS
#define TFT_RESET 4  //D4
#define TFT_DC 5     //D5
#define TFT_MOSI 11  //D11/MOSI
#define TFT_SCK 13   //D13/SCK
#define TFT_LED 3    //GPIO3
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
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2, 2, 2);

  gfx->setCursor(10, 10);
  gfx->println("Arduino Uno R4 Minima");

  gfx->setCursor(10, 30);
  gfx->println("+ ILI9488 SPI TFT");

  gfx->setCursor(10, 50);
  gfx->println("using Arduino_GFX Library");

  int w = gfx->width();
  int h = gfx->height();

  gfx->setCursor(10, 70);
  // gfx->printf("%i x %d", w, h);


  gfx->drawRect(0, 0, w, h, WHITE);

  delay(3000);

  for (int i = 0; i < w; i++) {
    int d = (int)(255 * i / w);
    gfx->drawLine(i, 0, i, w, RGB565(d, 0, 0));
    delay(10);
  }
  for (int i = 0; i < w; i++) {
    int d = (int)(255 * i / w);
    gfx->drawLine(w - i, 0, w - i, w, RGB565(0, d, 0));
    delay(10);
  }
  for (int i = 0; i < w; i++) {
    int d = (int)(255 * i / w);
    gfx->drawLine(i, 0, i, w, RGB565(0, 0, d));
    delay(10);
  }
}

void loop() {
  gfx->setTextColor(WHITE);
  gfx->setTextSize(6, 6, 2);

  gfx->fillScreen(RED);
  gfx->setCursor(100, 100);
  // gfx->printf("RED");
  gfx->println("RED");

  delay(2000);

  gfx->fillScreen(GREEN);
  gfx->setCursor(100, 100);
  // gfx->printf("GREEN");
  gfx->println("GREEN");
  delay(2000);

  gfx->fillScreen(BLUE);
  gfx->setCursor(100, 100);
  // gfx->printf("BLUE");
  gfx->println("BLUE");
  delay(2000);
}