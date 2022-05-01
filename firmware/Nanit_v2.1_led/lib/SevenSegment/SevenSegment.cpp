/*!
 * @file SevenSegment.cpp
 *
 * @mainpage SevenSegment library for Arduino.
 *
 * @section intro_sec Introduction
 *
 * Trimmed down version of the Adafruit_LEDBackpack library
 * 
 * This is an Arduino library for our I2C LED Backpacks:
 * ----> http://www.adafruit.com/products/
 * ----> http://www.adafruit.com/products/
 *
 * These displays use I2C to communicate, 2 pins are required to
 * interface. There are multiple selectable I2C addresses. For backpacks
 * with 2 Address Select pins: 0x70, 0x71, 0x72 or 0x73. For backpacks
 * with 3 Address Select pins: 0x70 thru 0x77
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * This library depends on <a
 * href="https://github.com/adafruit/Adafruit-GFX-Library"> Adafruit_GFX</a>
 * being present on your system. Please make sure you have installed the latest
 * version before using this library.
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 *
 * @section license License
 *
 * MIT license, all text above must be included in any redistribution
 *
 */

#include <Wire.h>

#include "SevenSegment.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) ///< Bit-value if not defined by Arduino
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  } ///< 16-bit var swap
#endif

static const uint8_t numbertable[] = {
    0x3F, /* 0 */
    0x06, /* 1 */
    0x5B, /* 2 */
    0x4F, /* 3 */
    0x66, /* 4 */
    0x6D, /* 5 */
    0x7D, /* 6 */
    0x07, /* 7 */
    0x7F, /* 8 */
    0x6F, /* 9 */
    0x77, /* a */
    0x7C, /* b */
    0x39, /* C */
    0x5E, /* d */
    0x79, /* E */
    0x71, /* F */
};

void SevenSegment::setBrightness(uint8_t b) {
  if (b > 15)
    b = 15;
  Wire.beginTransmission(i2c_addr);
  Wire.write(HT16K33_CMD_BRIGHTNESS | b);
  Wire.endTransmission();
}

void SevenSegment::blinkRate(uint8_t b) {
  Wire.beginTransmission(i2c_addr);
  if (b > 3)
    b = 0; // turn off if not sure

  Wire.write(HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON | (b << 1));
  Wire.endTransmission();
}

SevenSegment::SevenSegment(void) {}

void SevenSegment::begin(uint8_t _addr) {
  i2c_addr = _addr;

  Wire.begin();

  Wire.beginTransmission(i2c_addr);
  Wire.write(0x21); // turn on oscillator
  Wire.endTransmission();

  // internal RAM powers up with garbage/random values.
  // ensure internal RAM is cleared before turning on display
  // this ensures that no garbage pixels show up on the display
  // when it is turned on.
  clear();
  writeDisplay();

  blinkRate(HT16K33_BLINK_OFF);

  setBrightness(15); // max brightness
}

void SevenSegment::end() {
  clear();
  writeDisplay();
  Wire.beginTransmission(i2c_addr);
  Wire.write(0x20); // turn off oscillator
  Wire.endTransmission();
  Wire.end();
}

void SevenSegment::writeDisplay(void) {
  Wire.beginTransmission(i2c_addr);
  Wire.write((uint8_t)0x00); // start at address $00

  for (uint8_t i = 0; i < 8; i++) {
    Wire.write(displaybuffer[i]);
    Wire.write(0x00);
  }
  Wire.endTransmission();
}

void SevenSegment::clear(void) {
  for (uint8_t i = 0; i < 8; i++) {
    displaybuffer[i] = 0;
  }
}

void SevenSegment::print(unsigned long n, int base) {
  if (base == 0)
    write(n);
  else
    printNumber(n, base);
}

void SevenSegment::print(char c, int base) { print((long)c, base); }

void SevenSegment::print(unsigned char b, int base) {
  print((unsigned long)b, base);
}

void SevenSegment::print(int n, int base) { print((long)n, base); }

void SevenSegment::print(unsigned int n, int base) {
  print((unsigned long)n, base);
}

void SevenSegment::println(void) { position = 0; }

void SevenSegment::println(char c, int base) {
  print(c, base);
  println();
}

void SevenSegment::println(unsigned char b, int base) {
  print(b, base);
  println();
}

void SevenSegment::println(int n, int base) {
  print(n, base);
  println();
}

void SevenSegment::println(unsigned int n, int base) {
  print(n, base);
  println();
}

void SevenSegment::println(long n, int base) {
  print(n, base);
  println();
}

void SevenSegment::println(unsigned long n, int base) {
  print(n, base);
  println();
}

void SevenSegment::println(double n, int digits) {
  print(n, digits);
  println();
}

void SevenSegment::print(double n, int digits) { printFloat(n, digits); }

size_t SevenSegment::write(uint8_t c) {

  uint8_t r = 0;

  if (c == '\n')
    position = 0;
  if (c == '\r')
    position = 0;

  if ((c >= '0') && (c <= '9')) {
    writeDigitNum(position, c - '0');
    r = 1;
  }

  position++;
  if (position == 2)
    position++;

  return r;
}

void SevenSegment::writeDigitRaw(uint8_t d, uint8_t bitmask) {
  if (d > 4)
    return;
  displaybuffer[d] = bitmask;
}

void SevenSegment::drawLeds(bool sense, bool uR, bool mR, bool alarm, bool time) {
  uint8_t val = 0;
  bitWrite(val, 0, sense);
  bitWrite(val, 1, uR);
  bitWrite(val, 2, mR);
  bitWrite(val, 3, alarm);
  bitWrite(val, 4, time);
  displaybuffer[2] = val;
}

void SevenSegment::drawSenseLed(bool state) {
  bitWrite(displaybuffer[2], 0, state);
}

void SevenSegment::drawUrLed(bool state) {
    bitWrite(displaybuffer[2], 1, state);
}

void SevenSegment::drawMrLed(bool state) {
    bitWrite(displaybuffer[2], 2, state);
}

void SevenSegment::drawAlarmLed(bool state) {
    bitWrite(displaybuffer[2], 3, state);
}

void SevenSegment::drawTimeLed(bool state) {
    bitWrite(displaybuffer[2], 4, state);
}


void SevenSegment::writeLeds(void) {
  Wire.beginTransmission(i2c_addr);
  Wire.write((uint8_t)0x04); // start at address $02
  Wire.write(displaybuffer[2]);
  Wire.write(0x00);
  Wire.endTransmission();
}

void SevenSegment::writeDigitNum(uint8_t d, uint8_t num, bool dot) {
  writeDigitRaw(d, numbertable[num] | (dot << 7));
}

void SevenSegment::print(long n, int base) { printNumber(n, base); }

void SevenSegment::printNumber(long n, uint8_t base) {
  printFloat(n, 0, base);
}

void SevenSegment::printFloat(double n, uint8_t fracDigits, uint8_t base) {
  uint8_t numericDigits = 4; // available digits on display
  bool isNegative = false;   // true if the number is negative

  // is the number negative?
  if (n < 0) {
    isNegative = true; // need to draw sign later
    --numericDigits;   // the sign will take up one digit
    n *= -1;           // pretend the number is positive
  }

  // calculate the factor required to shift all fractional digits
  // into the integer part of the number
  double toIntFactor = 1.0;
  for (int i = 0; i < fracDigits; ++i)
    toIntFactor *= base;

  // create integer containing digits to display by applying
  // shifting factor and rounding adjustment
  uint32_t displayNumber = n * toIntFactor + 0.5;

  // calculate upper bound on displayNumber given
  // available digits on display
  uint32_t tooBig = 1;
  for (int i = 0; i < numericDigits; ++i)
    tooBig *= base;

  // if displayNumber is too large, try fewer fractional digits
  while (displayNumber >= tooBig) {
    --fracDigits;
    toIntFactor /= base;
    displayNumber = n * toIntFactor + 0.5;
  }

  // did toIntFactor shift the decimal off the display?
  if (toIntFactor < 1) {
    printError();
  } else {
    // otherwise, display the number
    int8_t displayPos = 4;

    if (displayNumber) // if displayNumber is not 0
    {
      for (uint8_t i = 0; displayNumber || i <= fracDigits; ++i) {
        bool displayDecimal = (fracDigits != 0 && i == fracDigits);
        writeDigitNum(displayPos--, displayNumber % base, displayDecimal);
        if (displayPos == 2)
          writeDigitRaw(displayPos--, 0x00);
        displayNumber /= base;
      }
    } else {
      writeDigitNum(displayPos--, 0, false);
    }

    // display negative sign if negative
    if (isNegative)
      writeDigitRaw(displayPos--, 0x40);

    // clear remaining display positions
    while (displayPos >= 0)
      writeDigitRaw(displayPos--, 0x00);
  }
}

void SevenSegment::printError(void) {
  for (uint8_t i = 0; i < SEVENSEG_DIGITS; ++i) {
    writeDigitRaw(i, (i == 2 ? 0x00 : 0x40));
  }
}
