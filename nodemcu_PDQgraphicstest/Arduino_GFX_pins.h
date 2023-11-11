#if defined(__IMXRT1052__) || defined(__IMXRT1062__)
// PJRC Teensy 4.x
#define TFT_CS 39 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 41
#define TFT_RST 40
#define GFX_BL 22
#elif defined(ARDUINO_BLACKPILL_F411CE)
#define TFT_CS 4 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 3
#define TFT_RST 2
#define GFX_BL 1
#elif defined(TARGET_RP2040)
#define TFT_CS 17 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 27
#define TFT_RST 26
#define GFX_BL 28
#elif defined(ESP32) && (CONFIG_IDF_TARGET_ESP32)
#define TFT_CS 5  // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 27 // GFX_NOT_DEFINED for display without DC pin (9-bit SPI)
#define TFT_RST 33
#define GFX_BL 22
#elif defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S2)
#define TFT_CS 34 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 38
#define TFT_RST 33
#define GFX_BL 21
#elif defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
#define TFT_CS 40 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 41
#define TFT_RST 42
#define GFX_BL 48
#elif defined(ESP32) && (CONFIG_IDF_TARGET_ESP32C3)
#define TFT_CS 7 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 2
#define TFT_RST 1
#define GFX_BL 3
#elif defined(ESP8266)
#define TFT_CS 15 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 4
#define TFT_RST 2
#define GFX_BL 5
#elif defined(RTL8722DM)
#if defined(BOARD_RTL8720DN_BW16)
#define TFT_CS 9
#define TFT_DC 8
#define TFT_RST 6
#define GFX_BL 3
#elif defined(BOARD_RTL8722DM)
#define TFT_CS 18
#define TFT_DC 17
#define TFT_RST 22
#define GFX_BL 23
#elif defined(BOARD_RTL8722DM_MINI)
#define TFT_CS 12
#define TFT_DC 14
#define TFT_RST 15
#define GFX_BL 13
#else             // old version
#define TFT_CS 18 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 17
#define TFT_RST 2
#define GFX_BL 23
#endif
#elif defined(SEEED_XIAO_M0)
#define TFT_CS 3 // GFX_NOT_DEFINED for display without CS pin
#define TFT_DC 2
#define TFT_RST 1
#define GFX_BL 0
#else
// #define TFT_CS 9 // GFX_NOT_DEFINED for display without CS pin
// #define TFT_DC 8
// #define TFT_RST 7
// #define GFX_BL 6

#define TFT_CS 16    //GPIO16 - D0
#define TFT_RST 5  //GPIO5 - D1; NOTICE THE TFT_RST and not TFT_RESET
#define TFT_DC 4     //GPIO4 - D2
#define TFT_MOSI 13  //GPIO13/MOSI - D7
#define TFT_SCK 14   //GPIO14/SCK - D5
#define TFT_LED 0    //GPIO0 - D3
#define TFT_MISO -1  // not used for TFT

#define GFX_BL TFT_LED  // backlight pin
#endif