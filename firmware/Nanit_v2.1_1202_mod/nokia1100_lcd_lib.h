//***************************************************************************
//  File........: nokia1100_lcd_lib.h
//  Author(s)...: Chiper
//  URL(s)......: http://digitalchip.ru/
//  Adapted for Nokia 2660/2760 by MadOrc
//***************************************************************************

#ifndef _NOKIA1100_LCD_LIB_H_
#define _NOKIA1100_LCD_LIB_H_

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#ifndef EEMEM
#define EEMEM__attribute__ ((section (".eeprom")))
#endif


//******************************************************************************
// Настройка библиотеки
//***************************************************************************
//  Notice: Все управляющие контакты LCD-контроллера должны быть подключены к
//  одному и тому же порту на микроконтроллере
//***************************************************************************
// Порт, к которому подключен LCD-контроллер NOKIA 1100
#define PORT_LCD PORTB
#define PIN_LCD  PINB
#define DDR_LCD  DDRB

// Номера выводов порта, к которым подключены выводы LCD-контроллера
#define SCLK_LCD_PIN    5
#define SDA_LCD_PIN     3
#define CS_LCD_PIN      2
#define RST_LCD_PIN     4

// *****!!!!! Минимальная задержка, при которой работает мой LCD-контроллер
// *****!!!!! В параметрах проекта частота 4МГц, фактически Atmega работает на 8МГц
// *****!!!!! Подбирается под конкретный контроллер
#define NLCD_MIN_DELAY	1

//******************************************************************************
// Макросы и определения

#define SCLK_LCD_SET    PORT_LCD |= (1<<SCLK_LCD_PIN)
#define SDA_LCD_SET     PORT_LCD |= (1<<SDA_LCD_PIN)
#define CS_LCD_SET      PORT_LCD |= (1<<CS_LCD_PIN)
#define RST_LCD_SET     PORT_LCD |= (1<<RST_LCD_PIN)

#define SCLK_LCD_RESET  PORT_LCD &= ~(1<<SCLK_LCD_PIN)
#define SDA_LCD_RESET   PORT_LCD &= ~(1<<SDA_LCD_PIN)
#define CS_LCD_RESET    PORT_LCD &= ~(1<<CS_LCD_PIN)
#define RST_LCD_RESET   PORT_LCD &= ~(1<<RST_LCD_PIN)

#define CMD_LCD_MODE	0
#define DATA_LCD_MODE	1

#define INV_MODE_ON		0
#define INV_MODE_OFF	1

//******************************************************************************
// Прототипы функций

void nlcd_Init(unsigned char contrast);
void nlcd_Clear(void);
void nlcd_SendByte(char mode,unsigned char c);
void nlcd_Print(char *message,  unsigned char x, unsigned char y, unsigned char inv);
void nlcd_PrintBig(char *message, unsigned char x, unsigned char y);

void nlcd_GotoXY(char x,char y);

void nlcd_Inverse(unsigned char mode);

#endif /* _NOKIA1100_LCD_LIB_H_ */
