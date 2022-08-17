#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <stdint.h>
#include <stdio.h>

#include "structs.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void oled_msgLn(char *str, unsigned int length);
void oled_msg(const char *str, unsigned int length);
void oled_printf(const char *fmt, ...);
int oled_init(void);
void oled_print(const char *msg);

void oled_msg(const char *str, unsigned int length) {
  for(unsigned int i = 0; i < length; i++) {
    display.write(str[i]);
  }
  display.display();
}

void oled_msgLn(char *str, unsigned int length) {
  oled_msg(str, length);
  display.write("\n");
}

int oled_init(void) {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Adafruit_SSD1306::begin() failed"));
    return(-1);
  }
  display.clearDisplay();
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.setRotation(0);
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  oled_printf("Display started!");
  return(0);
}

void oled_printf(const char *fmt, ...) {
  char buf[256];
  memset((void *) buf, 0, sizeof(buf));
  va_list args;
  va_start(args, fmt);
  vsprintf(buf, fmt, args);
  va_end(args);
  oled_msg(buf, strlen(buf));
  return;
}


void oled_print(const char *msg) {
  oled_msg(msg, strlen(msg));
  return;
}