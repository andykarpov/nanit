#ifndef SoftSerialTx_h
#define SoftSerialTx_h

#include <inttypes.h>

class SoftSerialTx
{
  private:
    long _baudRate;
    uint16_t _tx_delay;
  uint8_t _transmitBitMask;
  volatile uint8_t *_transmitPortRegister;
  volatile uint8_t *_pcint_maskreg;
  uint8_t _pcint_maskvalue;
    void printNumber(unsigned long, uint8_t);
    uint16_t subtract_cap(uint16_t num, uint16_t sub);
    inline void tunedDelay(uint16_t delay);
    
  public:
    SoftSerialTx(uint8_t);
    void setTX(uint8_t tx);
    void begin(long);
    void end();
    void print(char);
    void print(const char[]);
    void print(uint8_t);
    void print(int);
    void print(unsigned int);
    void print(long);
    void print(unsigned long);
    void print(long, int);
    void println(void);
    void println(char);
    void println(const char[]);
    void println(uint8_t);
    void println(int);
    void println(long);
    void println(unsigned long);
    void println(long, int);
};

#endif
