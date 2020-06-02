//***************************************************************************
//  File........: nokia1100_lcd_lib.h
//  Author(s)...: Chiper
//  URL(s)......: http://digitalchip.ru/
//  Adapted for Nokia 2660/2760 by MadOrc
//***************************************************************************
#ifndef _NOKIA1100_LCD_FONT_H_
#define _NOKIA1100_LCD_FONT_H_

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

// Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734
#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif

#ifndef EEMEM
#define EEMEM__attribute__ ((section (".eeprom")))
#endif

//***************************************************************************
// ������ ��������: 5x8. ������������� ��� ������������� ����������� �������
static unsigned char EEMEM nlcd_Font[][5] ={
//static const unsigned char nlcd_Font[][5] PROGMEM ={
	{0x00, 0x00, 0x00, 0x00, 0x00},// (space)
	{0x00, 0x00, 0x5F, 0x00, 0x00},// !
	{0x00, 0x00, 0x3C, 0x3C, 0x3C}, //bl " 
//	{0x00, 0x07, 0x00, 0x07, 0x00},// "
	{0x1C, 0x1C, 0x7F, 0x7F, 0x7F},// battL // #	
	{0x7F, 0x7F, 0x7F, 0x3E, 0x00},// battR //$
//	{0x14, 0x7F, 0x14, 0x7F, 0x14},// #
//	{0x24, 0x2A, 0x7F, 0x2A, 0x12},// $
	{0x23, 0x13, 0x08, 0x64, 0x62},// %
	{0x41, 0x22, 0x14, 0x08, 0x00},// > //& 
//	{0x36, 0x49, 0x55, 0x22, 0x50},// &
	{0x00, 0x05, 0x03, 0x00, 0x00},// '
	{0x00, 0x00, 0x7F, 0x41, 0x41},// [ // (
	{0x41, 0x41, 0x7F, 0x00, 0x00},// ] // )
//	{0x00, 0x1C, 0x22, 0x41, 0x00},// (
//	{0x00, 0x41, 0x22, 0x1C, 0x00},// )
	{0x08, 0x2A, 0x1C, 0x2A, 0x08},// *
	{0x08, 0x08, 0x3E, 0x08, 0x08},// +
	{0x00, 0x50, 0x30, 0x00, 0x00},// ,
	{0x08, 0x08, 0x08, 0x08, 0x08},// -
	{0x00, 0x30, 0x30, 0x00, 0x00},// .
	{0x20, 0x10, 0x08, 0x04, 0x02},// /
	{0x3E, 0x51, 0x49, 0x45, 0x3E},// 0
	{0x00, 0x42, 0x7F, 0x40, 0x00},// 1
	{0x42, 0x61, 0x51, 0x49, 0x46},// 2
	{0x21, 0x41, 0x45, 0x4B, 0x31},// 3
	{0x18, 0x14, 0x12, 0x7F, 0x10},// 4
	{0x27, 0x45, 0x45, 0x45, 0x39},// 5
	{0x3C, 0x4A, 0x49, 0x49, 0x30},// 6
	{0x01, 0x71, 0x09, 0x05, 0x03},// 7
	{0x36, 0x49, 0x49, 0x49, 0x36},// 8
	{0x06, 0x49, 0x49, 0x29, 0x1E},// 9
	{0x00, 0x36, 0x36, 0x00, 0x00},// :
/*	{0x00, 0x56, 0x36, 0x00, 0x00},// ;
	{0x00, 0x08, 0x14, 0x22, 0x41},// <
	{0x14, 0x14, 0x14, 0x14, 0x14},// =
	{0x41, 0x22, 0x14, 0x08, 0x00},// >
	{0x02, 0x01, 0x51, 0x09, 0x06},// ?
	{0x32, 0x49, 0x79, 0x41, 0x3E},// @
	{0x7E, 0x11, 0x11, 0x11, 0x7E},// A
	{0x7F, 0x49, 0x49, 0x49, 0x36},// B
	{0x3E, 0x41, 0x41, 0x41, 0x22},// C
	{0x7F, 0x41, 0x41, 0x22, 0x1C},// D
	{0x7F, 0x49, 0x49, 0x49, 0x41},// E
	{0x7F, 0x09, 0x09, 0x01, 0x01},// F
	{0x3E, 0x41, 0x41, 0x51, 0x32},// G
	{0x7F, 0x08, 0x08, 0x08, 0x7F},// H
	{0x00, 0x41, 0x7F, 0x41, 0x00},// I
	{0x20, 0x40, 0x41, 0x3F, 0x01},// J
	{0x7F, 0x08, 0x14, 0x22, 0x41},// K
	{0x7F, 0x40, 0x40, 0x40, 0x40},// L
	{0x7F, 0x02, 0x04, 0x02, 0x7F},// M
	{0x7F, 0x04, 0x08, 0x10, 0x7F},// N
	{0x3E, 0x41, 0x41, 0x41, 0x3E},// O
	{0x7F, 0x09, 0x09, 0x09, 0x06},// P
	{0x3E, 0x41, 0x51, 0x21, 0x5E},// Q
	{0x7F, 0x09, 0x19, 0x29, 0x46},// R
	{0x46, 0x49, 0x49, 0x49, 0x31},// S
	{0x01, 0x01, 0x7F, 0x01, 0x01},// T
	{0x3F, 0x40, 0x40, 0x40, 0x3F},// U
	{0x1F, 0x20, 0x40, 0x20, 0x1F},// V
	{0x7F, 0x20, 0x18, 0x20, 0x7F},// W
	{0x63, 0x14, 0x08, 0x14, 0x63},// X
	{0x03, 0x04, 0x78, 0x04, 0x03},// Y
	{0x61, 0x51, 0x49, 0x45, 0x43},// Z
	{0x00, 0x00, 0x7F, 0x41, 0x41},// [
	{0x02, 0x04, 0x08, 0x10, 0x20},// "\"
	{0x41, 0x41, 0x7F, 0x00, 0x00},// ]
	{0x04, 0x02, 0x01, 0x02, 0x04},// ^
	{0x40, 0x40, 0x40, 0x40, 0x40},// _
	{0x00, 0x01, 0x02, 0x04, 0x00},// `
	{0x20, 0x54, 0x54, 0x54, 0x78},// a
	{0x7F, 0x48, 0x44, 0x44, 0x38},// b
	{0x38, 0x44, 0x44, 0x44, 0x20},// c
	{0x38, 0x44, 0x44, 0x48, 0x7F},// d
	{0x38, 0x54, 0x54, 0x54, 0x18},// e
	{0x08, 0x7E, 0x09, 0x01, 0x02},// f
	{0x08, 0x14, 0x54, 0x54, 0x3C},// g
	{0x7F, 0x08, 0x04, 0x04, 0x78},// h
	{0x00, 0x44, 0x7D, 0x40, 0x00},// i
	{0x20, 0x40, 0x44, 0x3D, 0x00},// j
	{0x00, 0x7F, 0x10, 0x28, 0x44},// k
	{0x00, 0x41, 0x7F, 0x40, 0x00},// l
	{0x7C, 0x04, 0x18, 0x04, 0x78},// m
	{0x7C, 0x08, 0x04, 0x04, 0x78},// n
	{0x38, 0x44, 0x44, 0x44, 0x38},// o
	{0x7C, 0x14, 0x14, 0x14, 0x08},// p
	{0x08, 0x14, 0x14, 0x18, 0x7C},// q
	{0x7C, 0x08, 0x04, 0x04, 0x08},// r
	{0x48, 0x54, 0x54, 0x54, 0x20},// s
	{0x04, 0x3F, 0x44, 0x40, 0x20},// t
	{0x3C, 0x40, 0x40, 0x20, 0x7C},// u
	{0x1C, 0x20, 0x40, 0x20, 0x1C},// v
	{0x3C, 0x40, 0x30, 0x40, 0x3C},// w
	{0x44, 0x28, 0x10, 0x28, 0x44},// x
	{0x0C, 0x50, 0x50, 0x50, 0x3C},// y
	{0x44, 0x64, 0x54, 0x4C, 0x44},// z
	{0x00, 0x08, 0x36, 0x41, 0x00},// {
	{0x00, 0x00, 0x7F, 0x00, 0x00},// |
	{0x00, 0x41, 0x36, 0x08, 0x00},// }
	{0x02, 0x01, 0x02, 0x04, 0x02},// ~
	{0x08, 0x1C, 0x2A, 0x08, 0x08} // <-
*/		

 	//�������� ������������� ��������� 866 �� ������� "�", ����� - ��������������

	{ 0x7E, 0x11, 0x11, 0x11, 0x7E },  // �
	{ 0x7F, 0x49, 0x49, 0x49, 0x33 },  // �
	{ 0x7F, 0x49, 0x49, 0x49, 0x36 },  // �
	{ 0x7F, 0x01, 0x01, 0x01, 0x03 },  // �
	{ 0xE0, 0x51, 0x4F, 0x41, 0xFF },  // �
	{ 0x7F, 0x49, 0x49, 0x49, 0x49 },  // �
	{ 0x77, 0x08, 0x7F, 0x08, 0x77 },  // �
	{ 0x49, 0x49, 0x49, 0x49, 0x36 },  // �
	{ 0x7F, 0x10, 0x08, 0x04, 0x7F },  // �
	{ 0x7C, 0x21, 0x12, 0x09, 0x7C },  // �
	{ 0x7F, 0x08, 0x14, 0x22, 0x41 },  // �
	{ 0x20, 0x41, 0x3F, 0x01, 0x7F },  // �
	{ 0x7F, 0x02, 0x0C, 0x02, 0x7F },  // �
	{ 0x7F, 0x08, 0x08, 0x08, 0x7F },  // �
	{ 0x3E, 0x41, 0x41, 0x41, 0x3E },  // �
	{ 0x7F, 0x01, 0x01, 0x01, 0x7F },  // �
	{ 0x7F, 0x09, 0x09, 0x09, 0x06 },  // �
	{ 0x3E, 0x41, 0x41, 0x41, 0x22 },  // �
	{ 0x01, 0x01, 0x7F, 0x01, 0x01 },  // �
	{ 0x27, 0x48, 0x48, 0x48, 0x3F },  // �
	{ 0x1C, 0x22, 0x7F, 0x22, 0x1C },  // �
	{ 0x63, 0x14, 0x08, 0x14, 0x63 },  // �
	{ 0x7F, 0x40, 0x40, 0x40, 0xFF },  // �
	{ 0x07, 0x08, 0x08, 0x08, 0x7F },  // �
	{ 0x7F, 0x40, 0x7F, 0x40, 0x7F },  // �
	{ 0x7F, 0x40, 0x7F, 0x40, 0xFF },  // �
	{ 0x01, 0x7F, 0x48, 0x48, 0x30 },  // �
	{ 0x7F, 0x48, 0x30, 0x00, 0x7F },  // �
	{ 0x7F, 0x48, 0x48, 0x30, 0x00 },  // �
	{ 0x22, 0x41, 0x49, 0x49, 0x3E },  // �
	{ 0x7F, 0x08, 0x3E, 0x41, 0x3E },  // �
	{ 0x46, 0x29, 0x19, 0x09, 0x7F },  // �
	{ 0x20, 0x54, 0x54, 0x54, 0x78 },  // �
	{ 0x3C, 0x4A, 0x4A, 0x49, 0x31 },  // �
	{ 0x7C, 0x54, 0x54, 0x28, 0x00 },  // �
	{ 0x7C, 0x04, 0x04, 0x04, 0x0C },  // �
	{ 0xE0, 0x54, 0x4C, 0x44, 0xFC },  // �
	{ 0x38, 0x54, 0x54, 0x54, 0x08 },  // �
	{ 0x6C, 0x10, 0x7C, 0x10, 0x6C },  // �
	{ 0x44, 0x44, 0x54, 0x54, 0x28 },  // �
	{ 0x7C, 0x20, 0x10, 0x08, 0x7C },  // �
	{ 0x78, 0x42, 0x24, 0x12, 0x78 },  // �
	{ 0x7C, 0x10, 0x28, 0x44, 0x00 },  // �
	{ 0x20, 0x44, 0x3C, 0x04, 0x7C },  // �
	{ 0x7C, 0x08, 0x10, 0x08, 0x7C },  // �
	{ 0x7C, 0x10, 0x10, 0x10, 0x7C },  // �
	{ 0x38, 0x44, 0x44, 0x44, 0x38 },  // �
	{ 0x7C, 0x04, 0x04, 0x04, 0x7C },  // �
	{ 0x7C, 0x14, 0x14, 0x14, 0x08 },  // �
	{ 0x38, 0x44, 0x44, 0x44, 0x44 },  // �
	{ 0x04, 0x04, 0x7C, 0x04, 0x04 },  // �
	{ 0x0C, 0x50, 0x50, 0x50, 0x3C },  // �
	{ 0x18, 0x24, 0x7E, 0x24, 0x18 },  // �
	{ 0x44, 0x28, 0x10, 0x28, 0x44 },  // �
	{ 0x7C, 0x40, 0x40, 0x40, 0xFC },  // �
	{ 0x0C, 0x10, 0x10, 0x10, 0x7C },  // �
	{ 0x7C, 0x40, 0x7C, 0x40, 0x7C },  // �
	{ 0x7C, 0x40, 0x7C, 0x40, 0xFC },  // �
	{ 0x04, 0x7C, 0x50, 0x50, 0x20 },  // �
	{ 0x7C, 0x50, 0x20, 0x00, 0x7C },  // �
	{ 0x7C, 0x50, 0x50, 0x20, 0x00 },  // �
	{ 0x28, 0x44, 0x54, 0x54, 0x38 },  // �
	{ 0x7C, 0x10, 0x38, 0x44, 0x38 },  // �
	{ 0x08, 0x54, 0x34, 0x14, 0x7C }   // �

};
static const unsigned char nlcd_12x16_Font[][24] PROGMEM ={
  // 0x20 - ������ (������ ����������).
//{  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

  // 0x30 - '0'.
{  0x00, 0xf8, 0xfe, 0x06, 0x03, 0x83, 0xc3, 0x63, 0x33, 0x1e, 0xfe, 0xf8,
  0x00, 0x07, 0x1f, 0x1e, 0x33, 0x31, 0x30, 0x30, 0x30, 0x18, 0x1f, 0x07},

  // 0x31 - '1'.
{  0x00, 0x00, 0x00, 0x0c, 0x0c, 0x0e, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x3f, 0x3f, 0x30, 0x30, 0x30, 0x00},

  // 0x32 - '2'.
{  0x00, 0x1c, 0x1e, 0x07, 0x03, 0x03, 0x83, 0xc3, 0xe3, 0x77, 0x3e, 0x1c,
  0x00, 0x30, 0x38, 0x3c, 0x3e, 0x37, 0x33, 0x31, 0x30, 0x30, 0x30, 0x30},

  // 0x33 - '3'.
{  0x00, 0x0c, 0x0e, 0x07, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0x7e, 0x3c,
  0x00, 0x0c, 0x1c, 0x38, 0x30, 0x30, 0x30, 0x30, 0x30, 0x39, 0x1f, 0x0e},

  // 0x34 - '4'.
{  0x00, 0xc0, 0xe0, 0x70, 0x38, 0x1c, 0x0e, 0x07, 0xff, 0xff, 0x00, 0x00,
  0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x3f, 0x3f, 0x03, 0x03},

  // 0x35 - '5'.
{  0x00, 0x3f, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0xe3, 0xc3, 0x83,
  0x00, 0x0c, 0x1c, 0x38, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1f, 0x0f},

  // 0x36 - '6'.
{  0x00, 0xc0, 0xf0, 0xf8, 0xdc, 0xce, 0xc7, 0xc3, 0xc3, 0xc3, 0x80, 0x00,
  0x00, 0x0f, 0x1f, 0x39, 0x30, 0x30, 0x30, 0x30, 0x30, 0x39, 0x1f, 0x0f},

  // 0x37 - '7'.
{  0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0xc3, 0xf3, 0x3f, 0x0f, 0x03,
  0x00, 0x00, 0x00, 0x00, 0x30, 0x3c, 0x0f, 0x03, 0x00, 0x00, 0x00, 0x00},

  // 0x38 - '8'.
{  0x00, 0x00, 0xbc, 0xfe, 0xe7, 0xc3, 0xc3, 0xc3, 0xe7, 0xfe, 0xbc, 0x00,
  0x00, 0x0f, 0x1f, 0x39, 0x30, 0x30, 0x30, 0x30, 0x30, 0x39, 0x1f, 0x0f},

  // 0x39 - '9'.
{  0x00, 0x3c, 0x7e, 0xe7, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xe7, 0xfe, 0xfc,
  0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x38, 0x1c, 0x0e, 0x07, 0x03, 0x00},
	
};
#endif /* _NOKIA1100_LCD_FONT_H_ */