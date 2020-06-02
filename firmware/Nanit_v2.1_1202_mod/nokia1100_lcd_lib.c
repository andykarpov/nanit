//***************************************************************************
//  File........: nokia1100_lcd_lib.h
//  Author(s)...: Chiper
//  URL(s)......: http://digitalchip.ru/
//  Adapted for Nokia 2660/2760 by MadOrc
//  Display (96 x 68 pixels)
//***************************************************************************

//  Notice: ��� ����������� �������� LCD-����������� ������ ���� ���������� �
//  ������ � ���� �� ����� �� ����������������
//***************************************************************************
#define F_CPU 8000000UL

#include "nokia1100_lcd_lib.h"
#include "nokia1100_lcd_font.h"	// ���������� ����� (����� �������� � ����������� ������)

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
// ������������� �����������
void nlcd_Init(unsigned char contrast)
{	

	// �������������� ���� �� ����� ��� ������ � LCD-������������
	DDR_LCD |= (1<<SCLK_LCD_PIN)|(1<<SDA_LCD_PIN)|(1<<CS_LCD_PIN)|(1<<RST_LCD_PIN);

	CS_LCD_RESET;
	RST_LCD_RESET;

	_delay_ms(5);            // ������� �� ����� 5�� ��� ��������� ����������(����� 5 �� ����� ����������)

	RST_LCD_SET;


/*
���������
(0xae); // disable display 
(0x90); // �������� 0x90 max, 0x9f min (0x80 - 0x9F)
(0xC8); // mirror Y axis (about X axis)
(0xA6); // ������� �� ������
(0xA7); // ������ �� �������
(0xA1); // �������� ��������� �� �
*/


nlcd_SendByte(CMD_LCD_MODE,0xE2); // *** SOFTWARE RESET 
_delay_ms(10);
nlcd_SendByte(CMD_LCD_MODE,0xAF); //
nlcd_SendByte(CMD_LCD_MODE,0xA4); //
nlcd_SendByte(CMD_LCD_MODE,0x2F); //
nlcd_SendByte(CMD_LCD_MODE,0xB0); //

nlcd_SendByte(CMD_LCD_MODE,contrast); //

nlcd_SendByte(CMD_LCD_MODE,0x10); //
nlcd_SendByte(CMD_LCD_MODE,0x00); //

	nlcd_Clear(); // clear LCD
}

//******************************************************************************
// ������� ������
void nlcd_Clear(void)
{
	nlcd_SendByte(CMD_LCD_MODE,0x40); // Y = 0
	nlcd_SendByte(CMD_LCD_MODE,0xB0);
	nlcd_SendByte(CMD_LCD_MODE,0x10); // X = 0
	nlcd_SendByte(CMD_LCD_MODE,0x00);
	
	for(unsigned int i=0;i<864;i++) nlcd_SendByte(DATA_LCD_MODE,0x00);
	
}


//******************************************************************************
// �������� ����� (������� ��� ������) �� LCD-����������
//  mode: CMD_LCD_MODE - �������� �������
//		  DATA_LCD_MODE - �������� ������
//  c: �������� ������������� �����
void nlcd_SendByte(char mode,unsigned char c)
{

    CS_LCD_RESET;
    SCLK_LCD_RESET;
   
    if(mode) SDA_LCD_SET;
	 else	 SDA_LCD_RESET;
    
    SCLK_LCD_SET;


	SPCR = (0<<SPIE)|(1<<SPE)|(0<<DORD)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA)|(0<<SPR1)|(0<<SPR0);	// Enable Hardware SPI
	SPSR |= (1<<SPI2X);

    SPDR = c;
    while(!(SPSR & (1<<SPIF)));
    SPCR = 0;
	c <<= 1;
	_delay_us(NLCD_MIN_DELAY);

    CS_LCD_SET;


}

//******************************************************************************
// ����� ������� �� LCD-���������� � ������� �����
void nlcd_Putc(unsigned char c, unsigned char x, unsigned char y, unsigned char inv)
{
	if (c>127) c=c-64; 	// ��������� ������� �������� � ��������� CP1251 � ������ ������
						// �������� ������� ASCII (������� � ���� 0x80)
	if (c>62) c-=69; // >

	nlcd_GotoXY(x,y);	
	for ( unsigned char i = 0; i < 5; i++ )
	{
//    	if (inv) nlcd_SendByte(DATA_LCD_MODE,~pgm_read_byte(&(nlcd_Font[c-32][i])));
//		   else nlcd_SendByte(DATA_LCD_MODE,pgm_read_byte(&(nlcd_Font[c-32][i])));
		if (inv) nlcd_SendByte(DATA_LCD_MODE,~eeprom_read_byte(&(nlcd_Font[c-32][i])));
		else nlcd_SendByte(DATA_LCD_MODE,eeprom_read_byte(&(nlcd_Font[c-32][i])));
    }
    
	if (inv) nlcd_SendByte(DATA_LCD_MODE,0xFF); // ����� ����� ��������� �� ����������� � 1 �������
	else nlcd_SendByte(DATA_LCD_MODE,0x00); // ����� ����� ��������� �� ����������� � 1 �������
}


//******************************************************************************

void nlcd_PutcBig(unsigned char c, unsigned char x, unsigned char y)
{
	unsigned char i, glyph;
	c-=0x30;
	nlcd_GotoXY(x,y);

	for ( i = 0; i < 24; i++ )
	{
		if (i==12) nlcd_GotoXY(x,y+1);
//		glyph = eeprom_read_byte(&nlcd_12x16_Font[c-32][i]);
		if (c==240)
			glyph = 0x00;
		else
			glyph = pgm_read_byte(&(nlcd_12x16_Font[c][i]));
	   	nlcd_SendByte(DATA_LCD_MODE,glyph);
    }

//	nlcd_SendByte(DATA_LCD_MODE,0x00); // ����� ����� ��������� �� ����������� � 1 �������
}


//******************************************************************************

void nlcd_Print(char *message, unsigned char x, unsigned char y, unsigned char inv)
{
	unsigned char cn=0;
	while (*message) // ����� ������ ��������� �����
	{ 
    	nlcd_Putc(*message++, x+cn, y, inv);
		cn+=6;
    }
}


void nlcd_PrintBig(char *message, unsigned char x, unsigned char y)
{ 
	unsigned char cn=0;
	while (*message) { nlcd_PutcBig(*message++,x+cn,y); cn+=13;}  // ����� ������ ��������� �����
}

//******************************************************************************
void nlcd_GotoXY(char x,char y)
{
	if(y > 9) y = 0;
	if(x > 96) x = 0;

	nlcd_SendByte(CMD_LCD_MODE,(0xB0|(y)));      // ��������� ������ �� Y: 0100 yyyy         
    nlcd_SendByte(CMD_LCD_MODE,(0x10|(x>>4)));      // ��������� ������ �� X: 0000 xxxx - ���� (x3 x2 x1 x0)
    nlcd_SendByte(CMD_LCD_MODE,(0x0F&(x))); // ��������� ������ �� X: 0010 0xxx - ���� (x6 x5 x4)
}

//******************************************************************************
// ������������� ����� �������� ����� ������. ������ �� ������ �� ����������, ������ �������������
void nlcd_Inverse(unsigned char mode)
{
	if (mode) nlcd_SendByte(CMD_LCD_MODE,0xA6);
	else nlcd_SendByte(CMD_LCD_MODE,0xA7);
}

