#include <avr/interrupt.h>
#include "Arduino.h"
#include "SoftSerialTx.h"
#include <util/delay_basic.h>

/******************************************************************************
 * Statics
 ******************************************************************************/
static uint8_t _transmitPin;
static int _bitDelay;

/******************************************************************************
 * Constructors
 ******************************************************************************/

SoftSerialTx::SoftSerialTx(uint8_t transmitPin)
{
  _transmitPin = transmitPin;
  _baudRate = 0;
}

void SoftSerialTx::setTX(uint8_t tx) {
  _transmitPin = tx;
}

/******************************************************************************
 * User API
 ******************************************************************************/

uint16_t SoftSerialTx::subtract_cap(uint16_t num, uint16_t sub) {
  if (num > sub)
    return num - sub;
  else
    return 1;
}

inline void SoftSerialTx::tunedDelay(uint16_t delay) { 
  _delay_loop_2(delay);
}

void SoftSerialTx::begin(long speed)
{
    uint16_t bit_delay = (F_CPU / speed) / 4;

  // 12 (gcc 4.8.2) or 13 (gcc 4.3.2) cycles from start bit to first bit,
  // 15 (gcc 4.8.2) or 16 (gcc 4.3.2) cycles between bits,
  // 12 (gcc 4.8.2) or 14 (gcc 4.3.2) cycles from last bit to stop bit
  // These are all close enough to just use 15 cycles, since the inter-bit
  // timings are the most critical (deviations stack 8 times)
  _tx_delay = subtract_cap(bit_delay, 15 / 4);

  pinMode(_transmitPin, OUTPUT);
  digitalWrite(_transmitPin, HIGH);

  _baudRate = speed;
  tunedDelay(_tx_delay); // if we were low this establishes the end
}

void SoftSerialTx::end() 
{
    pinMode(_transmitPin, INPUT);
}

void SoftSerialTx::print(uint8_t b)
{
  if (_baudRate == 0)
    return;
  byte mask;

  cli();  // turn off interrupts for a clean txmit

  digitalWrite(_transmitPin, LOW);  // startbit
  tunedDelay(_tx_delay);

  for (mask = 0x01; mask; mask <<= 1) {
    if (b & mask){ // choose bit
      digitalWrite(_transmitPin,HIGH); // send 1
    }
    else{
      digitalWrite(_transmitPin,LOW); // send 1
    }
    tunedDelay(_tx_delay);
  }
  
  digitalWrite(_transmitPin, HIGH);
  sei();  // turn interrupts back on. hooray!
  tunedDelay(_tx_delay);
}

void SoftSerialTx::print(const char *s)
{
  while (*s)
    print(*s++);
}

void SoftSerialTx::print(char c)
{
  print((uint8_t) c);
}

void SoftSerialTx::print(int n)
{
  print((long) n);
}

void SoftSerialTx::print(unsigned int n)
{
  print((unsigned long) n);
}

void SoftSerialTx::print(long n)
{
  if (n < 0) {
    print('-');
    n = -n;
  }
  printNumber(n, 10);
}

void SoftSerialTx::print(unsigned long n)
{
  printNumber(n, 10);
}

void SoftSerialTx::print(long n, int base)
{
  if (base == 0)
    print((char) n);
  else if (base == 10)
    print(n);
  else
    printNumber(n, base);
}

void SoftSerialTx::println(void)
{
  print('\r');
  print('\n');  
}

void SoftSerialTx::println(char c)
{
  print(c);
  println();  
}

void SoftSerialTx::println(const char c[])
{
  print(c);
  println();
}

void SoftSerialTx::println(uint8_t b)
{
  print(b);
  println();
}

void SoftSerialTx::println(int n)
{
  print(n);
  println();
}

void SoftSerialTx::println(long n)
{
  print(n);
  println();  
}

void SoftSerialTx::println(unsigned long n)
{
  print(n);
  println();  
}

void SoftSerialTx::println(long n, int base)
{
  print(n, base);
  println();
}

// Private Methods /////////////////////////////////////////////////////////////

void SoftSerialTx::printNumber(unsigned long n, uint8_t base)
{
  unsigned char buf[8 * sizeof(long)]; // Assumes 8-bit chars. 
  unsigned long i = 0;

  if (n == 0) {
    print('0');
    return;
  } 

  while (n > 0) {
    buf[i++] = n % base;
    n /= base;
  }

  for (; i > 0; i--)
    print((char) (buf[i - 1] < 10 ? '0' + buf[i - 1] : 'A' + buf[i - 1] - 10));
}