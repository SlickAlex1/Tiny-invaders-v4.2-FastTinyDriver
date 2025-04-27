#include <Arduino.h>
#include "tinyJoypadUtils.h"

#if defined(__AVR_ATtiny85__)
  #include "FastTinyDriver.h"
  #include "smallFont.h"
#else
  #include <Adafruit_SSD1306.h>
  Adafruit_SSD1306 display(128, 64, &Wire, -1);
  uint8_t* adafruitBuffer;
  #ifdef _ENABLE_SERIAL_SCREENSHOT_
    #include "SerialHexTools.h"
  #endif
#endif

uint16_t analogJoystickX;
uint16_t analogJoystickY;

void InitTinyJoypad()
{
#if defined(__AVR_ATtiny85__)
  SOUND_PORT_DDR &= ~( ( 1 << PB5) | ( 1 << PB3 ) | ( 1 << PB1 ) );
  SOUND_PORT_DDR |=  ( 1 << SOUND_PIN );
#else
  pinMode(LEFT_RIGHT_BUTTON, INPUT);
  pinMode(UP_DOWN_BUTTON, INPUT);
  pinMode(FIRE_BUTTON, INPUT);
  pinMode(SOUND_PIN, OUTPUT);
  Serial.begin(115200);
#endif
}

bool isLeftPressed()  { uint16_t x = analogRead(LEFT_RIGHT_BUTTON); return (x >= 750 && x < 950); }
bool isRightPressed() { uint16_t x = analogRead(LEFT_RIGHT_BUTTON); return (x > 500 && x < 750); }
bool isUpPressed()    { uint16_t y = analogRead(UP_DOWN_BUTTON);    return (y > 500 && y < 750); }
bool isDownPressed()  { uint16_t y = analogRead(UP_DOWN_BUTTON);    return (y >= 750 && y < 950); }
bool isFirePressed()  { return (digitalRead(FIRE_BUTTON) == 0); }

void waitUntilButtonsReleased() {
  while(isLeftPressed() || isRightPressed() || isUpPressed() || isDownPressed() || isFirePressed());
}
void waitUntilButtonsReleased(const uint8_t delayTime) {
  waitUntilButtonsReleased();
  _delay_ms(delayTime);
}
void readAnalogJoystick() {
  analogJoystickX = analogRead(LEFT_RIGHT_BUTTON);
  analogJoystickY = analogRead(UP_DOWN_BUTTON);
}
bool wasLeftPressed()  { return (analogJoystickX >= 750 && analogJoystickX < 950); }
bool wasRightPressed() { return (analogJoystickX > 500 && analogJoystickX < 750); }
bool wasUpPressed()    { return (analogJoystickY > 500 && analogJoystickY < 750); }
bool wasDownPressed()  { return (analogJoystickY >= 750 && analogJoystickY < 950); }
uint16_t getAnalogValueX() { return analogJoystickX; }
uint16_t getAnalogValueY() { return analogJoystickY; }

void __attribute__((noinline)) _variableDelay_us(uint8_t delayValue)
{
  while(delayValue-- != 0) { _delay_us(1); }
}
void Sound(const uint8_t freq, const uint8_t dur)
{
  for (uint8_t t = 0; t < dur; t++)
  {
#if defined(__AVR_ATtiny85__) 
    if (freq != 0 ) { SOUND_PORT |= (1 << SOUND_PIN); }
    _variableDelay_us(255 - freq);
    SOUND_PORT &= ~(1 << SOUND_PIN);
    _variableDelay_us(255 - freq);
#else
    if (freq != 0 ) { digitalWrite(SOUND_PIN, 1); }
    _variableDelay_us(255 - freq);
    digitalWrite(SOUND_PIN, 0);
    _variableDelay_us(255 - freq);
#endif
  }
}

// ------- DISPLAY UTILS (with FastTinyDriver+smallFont for ATtiny85) -------

#if defined(__AVR_ATtiny85__)
// ASCII to font index translation for your characterFont3x5
int smallFont_lookup(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c == ';') return 10;  // UFO center
    if (c == ':') return 11;  // UFO right
    if (c == '<') return 12;
    if (c == '=') return 13;
    if (c == '>') return 14;
    if (c == '?') return 15;
    if (c == '!') return 16;
    if (c >= 'A' && c <= 'Z') return 17 + (c - 'A');
    // ...you can add more as needed
    return 0; // fallback to '0'
}

void oled_write_char_small(char c)
{
    int idx = smallFont_lookup(c);
    for (uint8_t i = 0; i < 4; i++)
        i2c_write(pgm_read_byte(&(characterFont3x5[idx * 4 + i])));
    i2c_write(0x00); // Space between chars
}
void oled_write_string_small(const char* str)
{
    while (*str) oled_write_char_small(*str++);
}

void oled_clear()
{
    for (uint8_t page = 0; page < 8; page++) {
        ssd1306_selectPage(page);
        for (uint8_t col = 0; col < 128; col++)
            i2c_write(0x00);
        i2c_stop();
    }
}
void oled_setpos(uint8_t x, uint8_t page)
{
    ssd1306_selectPage(page);
    for (uint8_t i = 0; i < x; i++) i2c_write(0x00);
}
#endif

// ---- Demo/compat display wrappers ----

void InitDisplay()
{
#if defined(__AVR_ATtiny85__)
  TinyOLED_init();
#else
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed - 1024 bytes for frame buffer required!")); for(;;);
  }
#endif
}

void PrepareDisplayRow(uint8_t y)
{
#if defined(__AVR_ATtiny85__)
  ssd1306_selectPage(y);
#else
  adafruitBuffer = display.getBuffer() + (y * 128);
#endif
}

void SendPixels(uint8_t pixels)
{
#if defined(__AVR_ATtiny85__)
  i2c_write(pixels);
#else
  *adafruitBuffer++ = pixels;
#endif
}

void FinishDisplayRow()
{
#if defined(__AVR_ATtiny85__)
  i2c_stop();
#endif
}

void DisplayBuffer()
{
#if !defined(__AVR_ATtiny85__)
  display.display();
  #ifndef _SERIAL_SCREENSHOT_NO_AUTO_SHOT_
    CheckForSerialScreenshot();
  #endif
#endif
}

void SerialScreenshot()
{
#if !defined(__AVR_ATtiny85__)
  #ifdef _ENABLE_SERIAL_SCREENSHOT_
    Serial.println(F("\r\nThis is a TinyJoypad screenshot..."));
    printScreenBufferToSerial(display.getBuffer(), 128, 8);
  #endif
#endif
}

void CheckForSerialScreenshot()
{
#if !defined(__AVR_ATtiny85__)
  #ifdef _ENABLE_SERIAL_SCREENSHOT_
    if (_SERIAL_SCREENSHOT_TRIGGER_CONDITION_)
      SerialScreenshot();
  #endif
#endif
}

#ifdef USE_SERIAL_PRINT
void serialPrint(const char *text) { Serial.print(text); }
void serialPrintln(const char *text) { Serial.println(text); }
void serialPrint(const __FlashStringHelper *text) { Serial.print(text); }
void serialPrintln(const __FlashStringHelper *text) { Serial.println(text); }
void serialPrint(const unsigned int number) { Serial.print(number); }
void serialPrintln(const unsigned int number) { Serial.println(number); }
void serialPrint(const int number) { Serial.print(number); }
void serialPrintln(const int number) { Serial.println(number); }
#else
void serialPrint(const char *text) { }
void serialPrintln(const char *text) { }
void serialPrint(const __FlashStringHelper *text) { }
void serialPrintln(const __FlashStringHelper *text) { }
void serialPrint(const unsigned int number) { }
void serialPrintln(const unsigned int number) { }
void serialPrint(const int number) { }
void serialPrintln(const int number) { }
#endif