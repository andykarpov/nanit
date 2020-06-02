/*
 * nanit.h
 *
 * Created: 2014
 * Author: MadOrc
 * Optimization: DooMmen
 * 
 * ������ 2.1 mod
 * Updated: 2020-06-02 andykarpov
 */ 

#define F_CPU 8000000UL


#define SBM10	0
#define SBM21	1
#define SBM20	32
#define SI19	33
#define SBM21_2	34	// 2 �������� ���-21
#define	M_SEL	40	// ����� ����� ��������� SBM10/SBM21

// ���������� ������������ ������! SI19 ��� SBM20 ��� SBM21 ��� SBM21_2 ��� SBM10
#define COUNTER_D SBM10 //M_SEL//SBM21

// ���� ���������� - ������ �� ��������� ������������, ���� ������������ - ��������� 3 ������
#define LI_ION

// ���� ����������, �� � ���� ����� ������� ������� �� ����, ���� ���, �� �� ����
//#define DOSE

// ���� ����������, �� ������� ���������� �� �����������
#define MIRR

// ���� ����������, �� ������� 2 ������ ��� �������� � ������ ������ (��� � ������ ������)
#define OLD_DRAW

// ��������� ��������� ������� ������ (���������� ��������� ��� SBM20 � SI19)
//#define G_2_LINE

// ����� ������ �������� ���� - �������������
//#define MAX_RAD_ON_MAIN


#if (COUNTER_D == SI19) || (COUNTER_D == SBM20)	// ������� ����� ������� ��-�� ���������� �������
	#define G_2_LINE
#endif

#if (COUNTER_D == M_SEL)
	#define OLD_DRAW
#endif

const unsigned char alarm_snooze_time=5;	// ����� � �������, ������� �� �������� �� ���� ��� ������� ����� ������� �� ������

//const unsigned char impulse_threshold=10;	// ����� �������� "������� ������ ����", �.�. ���� � 1� ������� ���� � ���������,
											// � ����� ������� �+impulse_threshold ��� X-impulse_threshold - �������� �������� ����

#ifdef LI_ION
	const unsigned int batt_warn=320;	// ����� ��������� "�������� �������" ��� ��������� ������ min=320
#else
	const unsigned char batt_warn=245;	// ����� ��������� "�������� ���������" min=245
#endif


#define OS_MAIN		__attribute__((OS_main))


#include <avr/io.h>
#include <util/delay.h>          // ���������� ��������
#include <avr/interrupt.h>
#include "nokia1100_lcd_lib.h"
#include <avr/eeprom.h>


#define	BEEP_1		PORTC |= (1<<PORTC2);	PORTC &= ~(1<<PORTC3)
#define	BEEP_2		PORTC &= ~(1<<PORTC2);	PORTC |= (1<<PORTC3)
#define	BEEP_OFF	PORTC &= ~ (1<<PORTC2);	PORTC &= ~ (1<<PORTC3)

const uint32_t  pow10Table32[]=
{
	100000ul,
	10000ul,
	1000ul,
	100ul,
	10ul,
	1ul
};


#ifndef EEMEM
	#define EEMEM__attribute__ ((section (".eeprom")))
#endif


volatile unsigned char hv_status, batt=0, count_flag=0;
char txt[7];
unsigned char awaken=30, light=30, go_to_bed=0, out_of_bed=0, alarm_snooze=0, alarm_beep=0, fonerr=0, displaying_dose=0;
unsigned char go_shutdown=0, do_shutdown=0, recount=0; // alarm=0, 
unsigned char second=0, minute=0, hour=12, battery_percent, redraw_LCD=0, blink_status=0, piip=0, scr=0, key=0, punkt=1;
volatile unsigned int pumpbreak;
unsigned char rad_counted=0, idle_pump=0;

#if (COUNTER_D == SBM21)
	unsigned int sbm[142]; //������ ���������
	unsigned int sbm_count_time=420, count_validate=420;
#elif (COUNTER_D == SBM21_2)
	unsigned int sbm[110]; //������ ���������
	unsigned char sbm_count_time=216, count_validate=216;
#elif (COUNTER_D == SI19)
	unsigned int sbm[50]; //������ ���������
	unsigned char sbm_count_time=64, count_validate=64;
#elif (COUNTER_D == SBM20)
	unsigned int sbm[50]; //������ ���������
	unsigned char sbm_count_time=44, count_validate=44;
#elif (COUNTER_D == SBM10)
	unsigned int sbm[112]; //������ ���������
	unsigned char sbm_count_time=220, count_validate=220;
#elif (COUNTER_D == M_SEL)
	unsigned int sbm[142]; //������ ���������
	unsigned int sbm_count_time, count_validate;
#else
	#error *** �� ��������� ��� �������/GM tube type not defined ***
#endif

unsigned long fon=0, rad=0;
unsigned long fon_daily=0, fonall=0, fonsecond=0, div_graph_sbm=1, radmax=0;
unsigned int beep_counter=0, beep_length, days=0;
volatile unsigned int VL;
unsigned int VoltLevel;

unsigned char light_level, sleep_level, beep_level, ion, inverse_level, contrast_level, impulse;
#if (COUNTER_D == M_SEL)
unsigned char counter;
#endif
unsigned int alarm_level;

unsigned char EEMEM e_light_level=10, e_sleep_level=30, e_beep_level=1, e_ion=110, e_inverse_level=0, e_contrast_level=0x90;
unsigned char EEMEM e_impulse=2, e_counter=SBM21;
unsigned int EEMEM e_alarm_level=80;
