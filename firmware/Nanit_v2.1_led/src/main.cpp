/**
 * Nanit v3.0
 * 
 * Custom firmware for 4-digit LED + ht16k33 driver
 * 
 * @author MadOrd
 * @author andykarpov
 */

#include <Arduino.h>
#include <EEPROM.h>
#include "SevenSegment.h"
//#include <SoftwareSerial.h>
#include "config.h"

SevenSegment matrix = SevenSegment();

//SoftwareSerial mySerial(0, 1); 

#define	BEEP_1		PORTC |= (1<<PORTC2);	PORTC &= ~(1<<PORTC3)
#define	BEEP_2		PORTC &= ~(1<<PORTC2);	PORTC |= (1<<PORTC3)
#define	BEEP_OFF	PORTC &= ~ (1<<PORTC2);	PORTC &= ~ (1<<PORTC3)

const uint8_t alarm_snooze_time = 5; // время в минутах, сколько не выводить ни один тип тревоги после нажатия на кнопку

#if BATTERY_TYPE == BATTERY_LION
	const uint16_t batt_warn = 320;	 // порог сообщения "Зарядите батарею" для литиевого аккума min=320
#else
	const uint16_t batt_warn = 245;	 // порог сообщения "Замените батарейки" min=245
#endif

const uint32_t pow10Table32[]=
{
	100000ul,
	10000ul,
	1000ul,
	100ul,
	10ul,
	1ul
};

volatile uint8_t hv_status, batt=0, count_flag=0;
char txt[7];
uint8_t awaken = 30, go_to_bed = 0, out_of_bed = 0, alarm_snooze = 0, alarm_beep = 0, fonerr = 0;
uint8_t go_shutdown = 0, do_shutdown = 0, recount = 0;  
uint8_t second = 0, minute = 0, hour = 12, battery_percent, lcd_redraw = 0, blink_status = 0, piip = 0;
volatile uint16_t pumpbreak;
uint8_t rad_counted = 0, idle_pump = 0;

#if (SENSOR_TYPE == SENSOR_SBM21)
	uint16_t sbm[142]; //массив импульсов
	uint16_t sbm_count_time=420, count_validate=420;
#elif (SENSOR_TYPE == SENSOR_SBM21_2)
	uint16_t sbm[110]; //массив импульсов
	uint16_t sbm_count_time=216, count_validate=216;
#elif (SENSOR_TYPE == SENSOR_SI19)
	uint16_t sbm[50]; //массив импульсов
	uint16_t sbm_count_time=64, count_validate=64;
#elif (SENSOR_TYPE == SENSOR_SBM20)
	uint16_t sbm[50]; //массив импульсов
	uint16_t sbm_count_time=44, count_validate=44;
#elif (SENSOR_TYPE == SENSOR_SBM10)
	uint16_t sbm[112]; //массив импульсов
	uint16_t sbm_count_time=220, count_validate=220;
#elif (SENSOR_TYPE == SENSOR_M_SEL)
	uint16_t sbm[142]; //массив импульсов
	uint16_t sbm_count_time, count_validate;
#else
	#error *** Не определен тип датчика/GM tube type not defined ***
#endif

unsigned long fon = 0, rad = 0;
unsigned long fon_daily = 0, fonall = 0, fonsecond = 0, div_graph_sbm = 1, radmax = 0;
unsigned int beep_counter = 0, beep_length, days = 0;
volatile unsigned int VL = 0;
unsigned int VoltLevel = 0;

uint8_t sleep_level, beep_level, brightness_level, ion, impulse;
uint8_t counter = 0;
unsigned int alarm_level;

enum screen_state_e {
	state_main = 0,
	state_menu,
	state_settings,
	state_alarm,
	state_low_battery,
	state_error,
	state_boot
};
screen_state_e scr = state_main;

enum button_state_e {
	state_key_1 = 1,
	state_key_2,
	state_key_none
};
button_state_e key = state_key_none;

enum menu_state_e {
	state_menu_alarm = 1,
	state_menu_sleep,
	state_menu_impulse,
	state_menu_brightness,
	state_menu_settings,
	state_menu_reset_doze,
	state_menu_power_off
};
menu_state_e menu = state_menu_alarm;

enum settings_state_e {
	state_settings_hours = 1,
	state_settings_minutes,
	state_settings_ion,
	state_settings_pump,
	state_settings_sensor
};
settings_state_e settings = state_settings_hours;

enum main_state_e {
	state_main_current = 0,
	state_main_time,
	state_main_battery
};

main_state_e displaying_dose = state_main_current;

void beep(unsigned char tone, unsigned char duration);
static void pump();
void lcd_clear();
void pins_on();
void pins_off();

//////////////////////////////////////////////////////////////////////////
void(* resetMC) (void) = 0; // Reset MC

//////////////////////////////////////////////////////////////////////////
EMPTY_INTERRUPT (ADC_vect)

//////////////////////////////////////////////////////////////////////////
ISR(INT0_vect) // Кнопка1
{
	SMCR &=~ (1 << SE); // сны запрещены
	if (do_shutdown)
	{
		do_shutdown = 0;
		resetMC();
	}

	if (sleep_level) awaken = sleep_level+1; else awaken = 61;
	scr = state_main; // Если уснули в меню - проснемся на главном
	out_of_bed = 1;
	piip = 0;
	beep(BEEP_TONE_ALARM, 20);
	displaying_dose = state_main_current;
	key = state_key_none; // пустая обработка, чтобы при выходе из сна, не реагировал на кнопку
}

//////////////////////////////////////////////////////////////////////////
ISR(INT1_vect) // импульсы с датчика 0
{
	PORTB &= ~(1<<PORTB0); //на всякий...
	sbm[0]++;
	piip = 1;
}

//////////////////////////////////////////////////////////////////////////
ISR(TIMER2_OVF_vect)	// обработчик прерывания по совпадению Т2
{                       
	PORTB &= ~(1<<PORTB0);	//на всякий...

	TCNT2 = 0xE0;			//обновим счетчик

	second++;				// увеличиваем секунды

	if (awaken ==1 ) go_to_bed = 1; // СПАТЬ!
	
	if ((awaken) && (sleep_level)) awaken--; //обратный отсчёт до сна

	if (count_validate>0) count_validate--;

	#if (SENSOR_TYPE == SENSOR_SBM21)	// 3!! секунды
		if (!count_flag)	// 0
		{
			div_graph_sbm = 1;
			for(uint8_t i = 139; i>0; i--) // k - не менее отображаемого графа [48]/[96]!
			{
				sbm[i] = sbm[i-1];
				if ((sbm[i] > div_graph_sbm) && (i < 96)) div_graph_sbm = sbm[i]; // Делитель графиков.
			}
			sbm[0] = 0;
		}
		if (count_flag<2) count_flag++; else count_flag = 0;

	#elif (SENSOR_TYPE == SENSOR_SBM21_2) // 2!! секунды
		if (count_flag)
		{
			div_graph_sbm = 1;
			for(uint8_t i = 107; i>0; i--) // k - не менее отображаемого графа [48]/[96]!
			{
				sbm[i] = sbm[i-1];
                if ((sbm[i] > div_graph_sbm) && (i < 96)) div_graph_sbm = sbm[i]; // Делитель графиков.
			}
			sbm[0] = 0;
		}
		if (count_flag) count_flag = 0; else count_flag = 1;

	#elif (SENSOR_TYPE == SENSOR_SI19) // 2 секунды
		if (count_flag)
		{
			div_graph_sbm = 1;
			for(uint8_t i=50; i>0; i--) // k - не менее отображаемого графа [48]!
			{
				sbm[i] = sbm[i-1];
				if ((sbm[i] > div_graph_sbm) && (i < 48)) div_graph_sbm = sbm[i]; // Делитель графиков.
			}
			sbm[0] = 0;
		}
		if (count_flag) count_flag = 0; else count_flag = 1;

	#elif (SENSOR_TYPE == SENSOR_SBM10)   // 2 секунды
		if (count_flag)
		{
			div_graph_sbm = 1;
			for(uint8_t i = 109; i>0; i--) // k - не менее отображаемого графа [48]/[96]!
			{
				sbm[i] = sbm[i-1];
                if ((sbm[i] > div_graph_sbm) && (i<96)) div_graph_sbm = sbm[i]; // Делитель графиков.
			}
			sbm[0] = 0;
		}
		if (count_flag) count_flag = 0; else count_flag = 1;

	#elif (SENSOR_TYPE == SENSOR_SBM20) // 1!!! секунда
		div_graph_sbm = 1;
		for(uint8_t i=50; i>0; i--) // k - не менее отображаемого графа [48]!
		{
			sbm[i] = sbm[i-1];
			if ((sbm[i] > div_graph_sbm) && (i < 48)) div_graph_sbm = sbm[i]; // Делитель графиков.
		}
		sbm[0] = 0;

	#elif (SENSOR_TYPE == SENSOR_M_SEL)
		if (counter == SENSOR_SBM10)
		{
			if (count_flag<1) count_flag++; else count_flag=0;
		}
		else
		{
			if (count_flag<2) count_flag++; else count_flag=0;
		}
		if (!count_flag)
		{
			div_graph_sbm = 1;
			uint8_t m;
			if (counter == SENSOR_SBM10) m=109;
			else m=139;
			
            for(uint8_t i=m; i>0; i--) // k - не менее отображаемого графа [48]!
			{
				sbm[i] = sbm[i-1];
				if ((sbm[i] > div_graph_sbm) && (i < 96)) div_graph_sbm = sbm[i]; // Делитель графиков.
			}
		sbm[0] = 0;
		}
	#else
	#error *** Не определен тип датчика/GM tube type not defined ***
	#endif

	//if ((rad>radmax)&&(!count_validate)) radmax=rad; // Валидно - если измерено.
    if (rad > radmax) radmax = rad; // Забиваем на валидность
	fonsecond += rad; // 
	fon_daily = (fon_daily + fonsecond)/3600;
	if(second == 60)			// если second = 60, second = 0
	{  // second
		second = 0;
		minute++;				// увеличиваем минуты
		if (alarm_snooze) alarm_snooze--;
		if (!rad) fonerr++;
		if ((fonerr > 10) && (!alarm_snooze)) { lcd_clear(); scr = state_error; alarm_beep = 2; fonerr = 0;} // Ошибка датчика, нет импульсов 
		batt = 1;

		if(minute == 60)		// если minute = 60, minute = 0
		{  // minute
			minute = 0;
			hour++;				// увеличиваем часы

		if(hour == 24)			// если hour = 24, hour = 0
		{ // hour
			hour = 0;
			days++;
			fonall += fon_daily;
			fonsecond = 0;
			fon_daily = 0;
			radmax = 0;

		} // hour
		} // minute
		} // second

	if (second %2 == 0) pump();
	lcd_redraw = 1;
	recount = 1;
}

//////////////////////////////////////////////////////////////////////////
ISR(TIMER0_COMPA_vect)
{ //sound
	if (blink_status) 
	{
		BEEP_1;
		blink_status = 0;
	}
	else
	{
		BEEP_2;
		blink_status = 1;
	}

	if (beep_counter++ > beep_length)	// 
	{ 
		TIMSK0 &=~ (1 << OCIE0A); 
		BEEP_OFF;
		beep_counter = 0; 
		if (alarm_beep == 2)	{alarm_beep = 0; beep(BEEP_TONE_KEY1, 254);}
	}
}

void lcd_clear()
{
	matrix.clear();
}

//Инициализация дисплея
void lcd_init()
{
	matrix.begin(0x70);
	matrix.setBrightness(brightness_level);
}

void beep(unsigned char tone, unsigned char duration)
{
	if (beep_counter) return;
	//beep_length = duration * 10;
	//OCR0A = tone;
	beep_length = duration;
	OCR0A = tone;
	TIMSK0 |= (1 << OCIE0A);
}

static void pump()
{
	unsigned int pumpa;

	if (impulse > 11) impulse = 11;
	if (impulse < 1) impulse = 1;

	pumpbreak = 0;
	pumpa = 0;

	do
	{
		pumpbreak++;
		PORTB |= (1<<PORTB0);
		for (uint8_t r=0; r < impulse; r++) asm("nop");
		PORTB &= ~(1<<PORTB0);

		TCNT1=0;
		_delay_us(38);
		hv_status = TCNT1;

		if (hv_status) { PORTB &= ~(1<<PORTB0); pumpa=800; } // выходим //7
	}
	while (pumpa++ < 800);

	PORTB &= ~(1<<PORTB0);
}

void count_rad()
{
	volatile unsigned long fon = 0;

	#if (SENSOR_TYPE == SENSOR_SBM21)
		const unsigned int counter_base = 1400; // время датчика / 3 * 10
		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 3(ТРИ) СЕКУНДЫ !!!
		for (unsigned char i=1; i<140; i++)
		{
			fon += sbm[i];

			if (i == 5)
			{
				if(fon > 1800) // фон больше 46800 мкр/ч интервал расчета 15 сек
				{
					fon *= 28;
					if (sbm_count_time != 15) count_validate = 15;
					sbm_count_time = 15;
					i=140; //вылет из цикла, экономим такты
				}
			}
	
			if (i == 28)
			{
				if(fon > 200) //  фон больше 1000 мкр/ч интервал расчета 84 сек
				{
					fon *= 5;
					if (sbm_count_time != 84) count_validate = 84;
					sbm_count_time = 84;
					i = 140; //вылет из цикла, экономим такты
				}
			}

			if (i == 139) 
			{
				if (sbm_count_time != 420) count_validate = 420;
				sbm_count_time = 420;
			}
		}

	#elif (SENSOR_TYPE == SENSOR_SBM21_2)
		const unsigned int counter_base = 1080; // время датчика /2 * 10
		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 2(две) СЕКУНДЫ!!!
		for (uint8_t i=1; i<108; i++)
		{
			fon += sbm[i];

			if (i == 12)
			{
				if(fon > 1800) //фон больше 16 200 мкр/ч интервал расчета 24 сек
				{
					fon *= 9; //
					if (sbm_count_time != 24) count_validate = 24;
					sbm_count_time = 24;
					i = 108; //вылет из цикла, экономим такты
				}
			}

			if (i == 18)
			{
				if(fon > 200) // фон больше 1200 мкр/ч  интервал расчета 36 сек
				{
					fon *= 6;
					if (sbm_count_time != 36) count_validate = 36;
					sbm_count_time = 36;
					i = 108; //вылет из цикла, экономим такты
				}
			}

			if (i == 54)
			{
				if(fon > 100)// фон больше 200 мкр/ч  интервал расчета 108 сек
				{
					fon *= 2;
					if (sbm_count_time != 108) count_validate = 108;
					sbm_count_time = 108;
					i = 108; //вылет из цикла, экономим такты
				}
			}

			if (i == 107)
			{
				if (sbm_count_time != 216) count_validate = 216;
				sbm_count_time = 216;
			}
		}

	#elif (SENSOR_TYPE == SENSOR_SI19)
		const unsigned int counter_base = 320; // время датчика /2 * 10
		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 2(две) СЕКУНДЫ
		for (unsigned char i=1; i<32; i++)
		{
			fon += sbm[i];

			if (i == 2)
			{
				if(fon > 700) // фон больше 14000 мкр/ч интервал расчета 4 сек
				{
					fon *= 20;
					if (sbm_count_time != 4) count_validate = 4;
					sbm_count_time = 4;
					i = 32; //вылет из цикла, экономим такты
				}
			}

			if (i == 8)
			{
				if(fon > 300) // фон больше 1200 мкр/ч интервал расчета 16 сек
				{
					fon *= 4;
					if (sbm_count_time != 16) count_validate = 16;
					sbm_count_time = 16;
					i = 32; //вылет из цикла, экономим такты
				}
			}

			if (i == 16)
			{
				if(fon > 50) // фон больше 100 мкр/ч интервал расчета 32 сек
				{
					fon *= 2;
					if (sbm_count_time != 32) count_validate = 32;
					sbm_count_time = 32;
					i = 32; //вылет из цикла, экономим такты
				}
			}

			if (i == 31)
			{
				if (sbm_count_time != 64) count_validate = 64;
				sbm_count_time = 64;
			}
		}

	#elif (SENSOR_TYPE == SENSOR_SBM20)
		const unsigned int counter_base = 440; // время датчика /2 * 10
		// СБМ-20 датчик без коррекции жёсткости хода (голый) СБМ20 - 44 сек
		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 2(две) СЕКУНДЫ
		for (uint8_t i=1; i<44; i++)
		{
			fon += sbm[i];

			if (i == 6)
			{
				if(fon > 2000)// фон больше 5500 мкР.ч интервал расчета 4 сек
				{
					fon *= 11;
					if (sbm_count_time != 6) count_validate = 6;
					sbm_count_time = 6;
					i = 44; //вылет из цикла, экономим такты
				}
			}

			if (i == 22)
			{
				if(fon > 100) //  фон больше 100 мкР.ч интервал расчета 22 сек
				{
					fon *= 2;
					if (sbm_count_time != 22) count_validate = 22;
					sbm_count_time = 22;
					i = 44; //вылет из цикла, экономим такты
				}
			}

			if (i == 43)
			{
				if (sbm_count_time != 44) count_validate = 44;
				sbm_count_time = 44;
			}
		}

	#elif (SENSOR_TYPE == SENSOR_SBM10)
		const unsigned int counter_base = 1100; // время датчика / 2 * 10
		// СБМ-10 После долгих прикидок и раздумий - датчик без коррекции жёсткости хода (голый) СБМ10 - 220с [78(чувствительность известного нам датчика сбм-20)/12(чувствительность датчика сбм-10)*34(время счёта СБМ20 так все-таки правильнее)]
		// если будем корректировать к примеру свинцовой фольгой чувствительность к частицам низких энергий время будет другим

		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 2(две) СЕКУНДЫ 220
		for (uint8_t i=1; i<110; i++)
		{
			fon += sbm[i];

			if (i == 10)
			{
				if(fon > 18181) // фон больше 200 000 мкр/ч интервал расчета 20 сек
				{
					fon *= 11; //
					if (sbm_count_time != 20) count_validate = 20;
					sbm_count_time = 20;
					i = 110; //вылет из цикла, экономим такты
				}
			}

			if (i == 28)
			{
				if(fon > 20000) // фон больше 80 000 мкр/ч интервал расчета 55 сек
				{
					fon *= 4;
					if (sbm_count_time != 55) count_validate = 55;
					sbm_count_time = 55;
					i = 110; //вылет из цикла, экономим такты
				}
			}

			if (i == 55)
			{
				if(fon > 250) // фон больше 500 мкр/ч интервал расчета 110 сек
				{
					fon *= 2;
					if (sbm_count_time != 110) count_validate = 110;
					sbm_count_time = 110;
					i = 110; //вылет из цикла, экономим такты
				}
			}

			if (i == 109)
			{
				if (sbm_count_time != 220) count_validate = 220;
				sbm_count_time = 220;
			}
		}

	#elif (SENSOR_TYPE == SENSOR_M_SEL)
		if (counter==SENSOR_SBM10)
		{	// SBM10
			for (uint8_t i=1; i<110; i++)
			{
				fon += sbm[i];

				if (i == 10)
				{
					if(fon > 18181) // фон больше 200 000 мкр/ч интервал расчета 20 сек
					{
						fon *= 11; //
						if (sbm_count_time != 20) count_validate = 20;
						sbm_count_time = 20;
						i = 110; //вылет из цикла, экономим такты
					}
				}

				if (i == 28)
				{
					if(fon > 20000) // фон больше 80 000 мкр/ч интервал расчета 55 сек
					{
						fon *= 4;
						if (sbm_count_time != 55) count_validate = 55;
						sbm_count_time = 55;
						i = 110; //вылет из цикла, экономим такты
					}
				}

				if (i == 55)
				{
					if(fon > 250) // фон больше 500 мкр/ч интервал расчета 110 сек
					{
						fon *= 2;
						if (sbm_count_time != 110) count_validate = 110;
						sbm_count_time = 110;
						i = 110; //вылет из цикла, экономим такты
					}
				}

				if (i == 109)
				{
					if (sbm_count_time != 220) count_validate = 220;
					sbm_count_time = 220;
				}
			}
		}
		else
		{	// SBM21
			for (uint8_t i=1; i<140; i++)
			{
				fon += sbm[i];
				if (i == 5)
				{
					if(fon > 1800) // фон больше 46800 мкр/ч интервал расчета 15 сек
					{
						fon *= 28;
						if (sbm_count_time != 15) count_validate = 15;
						sbm_count_time = 15;
						i = 140; //вылет из цикла, экономим такты
					}
				}
	
				if (i == 28)
				{
					if(fon > 200) //  фон больше 1000 мкр/ч интервал расчета 84 сек
					{
						fon *= 5;
						if (sbm_count_time != 84) count_validate = 84;
						sbm_count_time = 84;
						i = 140; //вылет из цикла, экономим такты
					}
				}

				if (i == 139) 
				{
					if (sbm_count_time != 420) count_validate = 420;
					sbm_count_time = 420;
				}
			}
		}
	#else
	#error *** Не определен тип датчика/GM tube type not defined ***
	#endif

	#if (SENSOR_TYPE == SENSOR_SBM21)  // 3 секунды !!!
		if (count_validate && (sbm_count_time - count_validate > 4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
		{
			uint8_t approx = (sbm_count_time - count_validate) / 3;
			fon = 0;
			for (uint8_t i=1; i<approx+3; i++)
			{
				fon = fon + sbm[i];
			}
			fon = fon * counter_base / approx / 10; // *на 10 из-за 8битной целой математики, фигово тут сплавающей точкой, лучше даже не связываться, просто "сдвигаем" 10е доли
		}

	#elif (SENSOR_TYPE == SENSOR_SBM20)  // 1 секунда !!!
		if (count_validate && (sbm_count_time - count_validate > 4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
		{
			uint8_t approx = (sbm_count_time - count_validate);
			fon = 0;
			for (uint8_t i=1; i<approx+1; i++)
			{
				fon = fon + sbm[i];
			}
			fon = fon * counter_base / approx / 10; // *на 10 из-за 8битной целой математики, фигово тут сплавающей точкой, лучше даже не связываться, просто "сдвигаем" 10е доли
		}

	#elif (SENSOR_TYPE == SENSOR_M_SEL)
		uint8_t t = 1;
		unsigned int counter_base = 0;
		if (counter == SENSOR_SBM10){
			counter_base = 1100;
			t = 2;
        } else {
			counter_base = 1400;
			t = 3;
        }
		if (count_validate && (sbm_count_time - count_validate > 4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
		{
			uint8_t approx = (sbm_count_time - count_validate) / t;
			fon = 0;
			for (uint8_t i=1; i<approx+t; i++)
			{
				fon = fon + sbm[i];
			}
			fon = fon * counter_base / approx / 10;	// *на 10 из-за 8битной целой математики, фигово тут сплавающей точкой, лучше даже не связываться, просто "сдвигаем" 10е доли
		}

	#else // 2 секунды
		if (count_validate && (sbm_count_time - count_validate > 4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
		{
			uint8_t approx = (sbm_count_time - count_validate) / 2;
			fon = 0;
			for (uint8_t i=1; i<approx+2; i++)
			{
				fon = fon + sbm[i];
			}
			fon = fon * counter_base / approx / 10; // *на 10 из-за 8битной целой математики, фигово тут сплавающей точкой, лучше даже не связываться, просто "сдвигаем" 10е доли
		}
	#endif

	rad = fon;

	#if (ALARM_TYPE == ALARM_DOSE)
		if ((fon_daily / 1000 > alarm_level) && (!alarm_snooze) && (alarm_level))
	#else
		if ((rad > alarm_level) && (!alarm_snooze) && (alarm_level) && (sbm_count_time - count_validate > 5))
	#endif
	{
		if (!awaken) // если спим - ПОДЪЁМ!
		{
			SMCR &=~ (1 << SE); // сны запрещены
			awaken = 61;
			//light = light_level + 1;
		   	pins_on();
		   	EIMSK &=~ (1 << INT0);
		   	PORTB &= ~(1<<PORTB0); //на всякий...
		   	PORTC |= (1<<PORTC0); // питание дисплея вкл
			menu = state_menu_alarm;
			settings = state_settings_hours;
			lcd_init();
			lcd_clear();
			lcd_redraw = 1;
		}

		alarm_beep = 2; beep(BEEP_TONE_KEY1, 254);
		if (scr != state_alarm) lcd_clear();
			scr = state_alarm;
		lcd_redraw = 1;
	}
}


void lcd_draw_screen()
{
	switch(scr)
	{
		case state_main: // главный
		default:
			matrix.blinkRate(HT16K33_BLINK_OFF);
			unsigned long fonad;	//для расчета fonall + fon_daily

            switch (displaying_dose)
            { //dd

				case state_main_current:
                default:
					if (rad < 10000) {
						matrix.print(rad, DEC);
						matrix.drawLeds(beep_counter, true, false, false, false);
					} else {
						matrix.print((rad + 500)/1000, DEC);
						matrix.drawLeds(beep_counter, false, true, false, false);
					}
					break;
                
				case state_main_time:
					matrix.writeDigitNum(0, hour/10);
					matrix.writeDigitNum(1, hour %10, second % 2 == 0);
					matrix.writeDigitNum(3, minute/10);
					matrix.writeDigitNum(4, minute % 10);
					matrix.drawLeds(beep_counter, false, false, false, true);
                    break;

				case state_main_battery:
					matrix.writeDigitRaw(0, 0b11111100); // b.
					matrix.writeDigitNum(1, battery_percent / 100);
					matrix.writeDigitNum(3, (battery_percent % 100) / 10);
					matrix.writeDigitNum(4, (battery_percent % 100) % 10);
					matrix.drawLeds(beep_counter, false, false, false, false);
					break;
            } // dd

            break;

		case state_menu: // юзер меню

			matrix.blinkRate(HT16K33_BLINK_2HZ);

			switch (menu) {
				case state_menu_alarm:
					matrix.writeDigitRaw(0, 0b11110111); // A.

					if (alarm_level) {
						matrix.writeDigitNum(1, alarm_level / 100);
						matrix.writeDigitNum(3, (alarm_level % 100) / 10);
						matrix.writeDigitNum(4, (alarm_level % 100) % 10);
					} else {
						matrix.writeDigitRaw(1, 0b00111111); // O
						matrix.writeDigitRaw(3, 0b01110001); // F
						matrix.writeDigitRaw(4, 0b01110001); // F
					}
				break;
				case state_menu_sleep:
					matrix.writeDigitRaw(0, 0b11101101); // S.
					if (sleep_level) {
						matrix.writeDigitNum(1, sleep_level / 100);
						matrix.writeDigitNum(3, (sleep_level % 100) / 10);
						matrix.writeDigitNum(4, (sleep_level % 100) % 10);
					} else {
						matrix.writeDigitRaw(1, 0b00111111); // O
						matrix.writeDigitRaw(3, 0b01110001); // F
						matrix.writeDigitRaw(4, 0b01110001); // F
					}
				break;
				case state_menu_brightness:
					matrix.writeDigitRaw(0, 0b01111100); // b
					matrix.writeDigitRaw(1, 0b11010000); // r.
					matrix.writeDigitNum(3, brightness_level / 10);
					matrix.writeDigitNum(4, brightness_level % 10);
				break;
				case state_menu_impulse:
					matrix.writeDigitRaw(0, 0b11111100); // b.
					if (beep_level) {
						matrix.writeDigitRaw(1, 0b00111111); // O
						matrix.writeDigitRaw(3, 0b01010100); // n
						matrix.writeDigitRaw(4, 0b00000000); //
					} else {
						matrix.writeDigitRaw(1, 0b00111111); // O
						matrix.writeDigitRaw(3, 0b01110001); // F
						matrix.writeDigitRaw(4, 0b01110001); // F
					}
				break;
				case state_menu_settings:
					matrix.writeDigitRaw(0, 0b01101101); // S
					matrix.writeDigitRaw(1, 0b01111001); // E
					matrix.writeDigitRaw(3, 0b01111000); // t
					matrix.writeDigitRaw(4, 0b00000000); //
				break;
				case state_menu_reset_doze:
					matrix.writeDigitRaw(0, 0b01010000); // r
					matrix.writeDigitRaw(1, 0b01101101); // S
					matrix.writeDigitRaw(3, 0b01111000); // t
					matrix.writeDigitRaw(4, 0b00000000); //
				break;
				case state_menu_power_off:
					matrix.writeDigitRaw(0, 0b00111111); // O
					matrix.writeDigitRaw(1, 0b01110001); // F
					matrix.writeDigitRaw(3, 0b01110001); // F
					matrix.writeDigitRaw(4, 0b00000000); //
				break;
			}
			break;

		case state_settings:
			matrix.blinkRate(HT16K33_BLINK_2HZ);
			switch (settings) {
				case state_settings_hours:
					matrix.writeDigitRaw(0, 0b01110110); // H
					matrix.writeDigitRaw(1, 0b11110110); // H.
					matrix.writeDigitNum(3, hour / 10);
					matrix.writeDigitNum(4, hour % 10);
				break;
				case state_settings_minutes:
					matrix.writeDigitRaw(0, 0b00010000); // i
					matrix.writeDigitRaw(1, 0b10010000); // i.
					matrix.writeDigitNum(3, minute / 10);
					matrix.writeDigitNum(4, minute % 10);
				break;
				case state_settings_ion:
					matrix.writeDigitRaw(0, 0b10111110); // U.
					matrix.writeDigitNum(1, ion/100);
					matrix.writeDigitNum(3, (ion % 100) / 10);
					matrix.writeDigitNum(4, (ion % 100) % 10);
				break;
				case state_settings_pump:
					matrix.writeDigitRaw(0, 0b11110011); // P.
					matrix.writeDigitRaw(1, 0b00000000);
					matrix.writeDigitNum(3, impulse / 10);
					matrix.writeDigitNum(4, impulse % 10);
				break;
			#if (SENSOR_TYPE == SENSOR_M_SEL)
				case state_settings_sensor:
					matrix.writeDigitRaw(0, 0b11101101); // S.
					matrix.writeDigitRaw(1, 0b00000000);
					if (counter==SENSOR_SBM10) {
						matrix.writeDigitNum(3, 1);
						matrix.writeDigitNum(4, 0);
					} else {
						matrix.writeDigitNum(3, 2);
						matrix.writeDigitNum(4, 1);
					}
				break;
			#endif
			}
		break;
			
		case state_alarm:
			matrix.blinkRate(HT16K33_BLINK_2HZ);
			if (rad < 10000) {
				matrix.print(rad, DEC);
				matrix.drawLeds(beep_counter, true, false, true, false);
			} else {
				matrix.print((rad + 500)/1000, DEC);
				matrix.drawLeds(beep_counter, false, true, true, false);
			}
			break; 

		case state_low_battery:
			matrix.blinkRate(HT16K33_BLINK_HALFHZ);
			matrix.writeDigitRaw(0, 0b00111000); // L
			matrix.writeDigitRaw(1, 0b01011100); // o
			matrix.writeDigitRaw(3, 0b00011101); // w
			matrix.writeDigitRaw(4, 0b01111100); // b
		break;
		case state_error:
			matrix.blinkRate(HT16K33_BLINK_1HZ);
			matrix.writeDigitRaw(0, 0b01111001); // E
			matrix.writeDigitRaw(1, 0b01010000); // r
			matrix.writeDigitRaw(3, 0b01010000); // r
			matrix.writeDigitRaw(4, 0b00000000);
		break;

		case state_boot:
			matrix.blinkRate(HT16K33_BLINK_2HZ);
			matrix.writeDigitRaw(0, 0b01111100); // b
			matrix.writeDigitRaw(1, 0b01011100); // o
			matrix.writeDigitRaw(3, 0b01011100); // o
			matrix.writeDigitRaw(4, 0b01111000); // t
		break;
	}

	matrix.writeDisplay();
}


void check_key()
{
	if (key != state_key_none) return; // пока капу не обработали, новых не читаем

	if(digitalRead(PIN_BTN1) == LOW ) // если кнопка 1 нажата
	{
		key = state_key_1;
		beep(BEEP_TONE_ALARM, 20);
	}
	else if(digitalRead(PIN_BTN2) == LOW) // если кнопка 2 нажата
	{
		key = state_key_2;
		beep(BEEP_TONE_KEY2, 20);
	}
}


void process_key()
{
	if (sleep_level) awaken = sleep_level + 1; else awaken = 61; 

	switch (scr)
	{
		case state_main: // Мы на главном
			switch(key)
			{
				case state_key_none: // ничего не нажато
				break;

				case state_key_1: // переключение режима отображения на главном экране
					switch (displaying_dose) {
						case state_main_current: displaying_dose = state_main_time; break;
						case state_main_time: displaying_dose = state_main_battery; break;
						case state_main_battery: displaying_dose = state_main_current; break;
					}
					lcd_redraw  = 1;
					break;
				
				case state_key_2: // в меню
					scr = state_menu;
					menu = state_menu_alarm;
					lcd_clear();
					lcd_redraw = 1;
					break;
			}
			break;

		case state_menu: // Мы в меню
			switch(key)
			{
				case state_key_none: // ничего не нажато - ничего не делаем
				break;

				case state_key_1:
					// меняем чаво нада
					switch (menu)
					{
						case state_menu_alarm: //тревога
							switch (alarm_level)
							{
								#if ALARM_TYPE == ALARM_DOSE
									case 0:
										alarm_level=50;
										break;
									case 50:
										alarm_level=100;
										break;
									case 100:
										alarm_level=200;
										break;
									case 200:
										alarm_level=300;
										break;
									case 300:
										alarm_level=500;
										break;
									case 500:
										alarm_level=1000;
										break;
								#else
									case 0:
										alarm_level=40;
										break;
									case 40:
										alarm_level=80;
										break;
									case 80:
										alarm_level=120;
										break;
									case 120:
										alarm_level=500;
										break;
									//case 500:
									//	alarm_level=1000;
									//	break;
									//case 1000:
									//	alarm_level=3000;
									//	break;
								#endif
									default: // если 3000 или чтото не то -> 0
										alarm_level=0;
										break;
							}
							break;

						case state_menu_sleep: //сон 
							switch (sleep_level)
							{
								case 0:
									sleep_level=30;
									break;
								case 30:
									sleep_level=60;
									break;
								case 60:
									sleep_level=120;
									break;
								default:
									sleep_level=0;
									break;
							}
							break;

						case state_menu_brightness: //яркость 
							if (brightness_level < 15) brightness_level++;
							else brightness_level=1;
							matrix.setBrightness(brightness_level);
							break;

						case state_menu_impulse: //имп звук
							if (beep_level) beep_level=0;
							else beep_level=1;
							break;

						case state_menu_settings: //Перейти в настройки
							scr = state_settings;
							settings = state_settings_hours;
							lcd_clear();
							break;

						case state_menu_reset_doze: //Сброс дозы
							radmax=0;
							fonsecond=0;
							fon_daily=0;
							fonall=0;
							days=0;
							menu=state_menu_alarm;
							scr = state_main;
							lcd_clear();
							break;
						
						case state_menu_power_off: //Выключение
							go_shutdown=1;
							break;
						}
						break;
					
				case state_key_2: // скачем по менюшкам
					switch (menu)
					{
						case state_menu_alarm: menu = state_menu_sleep; break;
						case state_menu_sleep: menu = state_menu_impulse; break;
						case state_menu_impulse: menu = state_menu_brightness; break;
						case state_menu_brightness: menu = state_menu_settings; break;
						//case state_menu_settings: menu = state_menu_reset_doze; break;
						case state_menu_settings: menu = state_menu_power_off; break;
						//case state_menu_reset_doze: menu = state_menu_power_off; break;
						case state_menu_power_off: 
							// выходим в главное меню по достижению конца меню
							cli();
							EEPROM.put(EEPROM_ADDR_BEEP_LEVEL, beep_level);
							EEPROM.put(EEPROM_ADDR_SLEEP_LEVEL, sleep_level);
							EEPROM.put(EEPROM_ADDR_ALARM_LEVEL, alarm_level);
							EEPROM.put(EEPROM_ADDR_BRIGHTNESS_LEVEL, brightness_level);
							sei();
							scr = state_main;
							menu = state_menu_alarm;
							lcd_clear();
							break;
					}
					break;
				}
			lcd_redraw=1;
			break;

		case state_settings: // Мы в настройках
			switch(key)
			{
				case state_key_none:
				break;

				case state_key_1:
					switch (settings)
					{
						case state_settings_hours: //Часы
							if (hour<23) hour++; else hour=0;
							second=0;
							break;

							case state_settings_minutes: //Минуты
							if (minute<59) minute++; else minute=0;
							second=0;
							break;

							case state_settings_ion: //Подстройка вольтметра
							if ((ion<124)&&(ion>99)) ion++; else ion=100;
							batt=1;
							break;

							case state_settings_pump: //Подстройка накачки
							if ((impulse<11)&&(impulse>0)) impulse++; else impulse=1;
							break;

							case state_settings_sensor: //Выбор датчика
							if (counter == SENSOR_SBM10){
								counter = SENSOR_SBM21;
								sbm_count_time=420;}
							else{
								counter = SENSOR_SBM10;
								sbm_count_time=220;}
							count_validate = sbm_count_time;
							break;
					}
					break;

				case state_key_2: // скачем по менюшкам

					switch (settings)
					{
						case state_settings_hours: settings = state_settings_minutes; break;
						case state_settings_minutes: settings = state_settings_ion; break;
						case state_settings_ion: settings = state_settings_pump; break;
						case state_settings_pump: 
							#if (SENSOR_TYPE == SENSOR_M_SEL)
							settings = state_settings_sensor;
							#else 
								cli();
								EEPROM.put(EEPROM_ADDR_ION, ion);
								EEPROM.put(EEPROM_ADDR_IMPULSE, impulse);
								sei();
								scr = state_main;
								menu = state_menu_alarm;
								settings = state_settings_hours;
								lcd_clear();
							#endif
							break;
						case state_settings_sensor:
							cli();
							EEPROM.put(EEPROM_ADDR_ION, ion);
							EEPROM.put(EEPROM_ADDR_IMPULSE, impulse);
							EEPROM.put(EEPROM_ADDR_COUNTER, counter);
							sei();
							scr = state_main;
							menu = state_menu_alarm;
							settings = state_settings_hours;
							lcd_clear();
						break;
					}
					break;
			}
			lcd_redraw=1;
			break;
		
		case state_alarm: // Алярма
		case state_low_battery: // БатарЭя, Азиз
		case state_error: // Нет импульсов
		case state_boot: // Бутскрин
			scr = state_main;
			lcd_clear();
			// alarm=0; 
			alarm_snooze = alarm_snooze_time; // сколько минут ни о чём не алармить
			lcd_redraw=1;
			break;
	}
	_delay_ms(400);
	key = state_key_none;
}


void check_battery()
{
	unsigned char Temp;
	unsigned int VLcc;	

	// * * **Настройка АЦП**
	ADCSRA = (1 << ADEN)		//Включение АЦП
			|(0 << ADSC)		// можна мерять
			|(1 << ADIE)		// разрешить прерывание
			|(1 << ADPS2)|(1 << ADPS1)|(0 << ADPS0); // предделитель на 64 (частота АЦП 125kHz при шине 8mHz)

	ADMUX = (0 << REFS1)|(1 << REFS0) // опора AVCC
			|(1 << ADLAR)		// результат только в старший байт, ADCL не нужен
			|(0 << MUX0)|(1 << MUX1)|(1 << MUX2)|(1 << MUX3); // вход  вн. ИОН
	Temp=SMCR;
	SMCR = (1 << SE)|(1 << SM0)|(0 << SM1)|(0 << SM2); // Замер в режиме низких шумов

	_delay_us(190);				// даем время включится АЦП
	ADCSRA |= (1 << ADSC);		//Начинаем преобразование
	asm("sleep");				//баиньки-баю
	while (ADIF == 0){;}		//Ждем флага окончания преобразования
	for (unsigned char z=64; z>0; z--) //
	{
		ADCSRA |= (1 << ADSC);	//Начинаем преобразование
		asm("sleep");			//баиньки-баю
		while (ADIF == 0){;}	//Ждем флага окончания преобразования
	}

	ADCSRA = 0;					//Выключение АЦП
	SMCR = Temp;

	Temp = ADCH;

	VLcc = (int)((ion*255)/Temp);	// 1.1 ИОН можно уточнить вольтметром и формулой ADC_VALUE = V_BG * 255/Vcc)
	VoltLevel = VLcc;

	if ((VLcc<batt_warn)&&(!alarm_snooze)) { lcd_clear(); alarm_beep=2; scr=state_low_battery; } // при замере юзер увидит, нефиг расходовать на "биби" и свет остатки батареи

	#if BATTERY_TYPE == BATTERY_LION
		if (VLcc<320) VLcc=320;
		if (VLcc>420) VLcc=420;
		battery_percent=(VLcc-320);//*100/100//*183/256;//100/140;
	#else
		if (VLcc<245) VLcc=245;
		if (VLcc>300) VLcc=300;
		battery_percent=(VLcc-245)*100/55;
	#endif
}


void pins_on()
{
	PRR = (0<<PRTIM0)|(0<<PRADC)|(0<<PRSPI)|(0<<PRTWI);

	pinMode(PIN_BTN2, INPUT_PULLUP); 	// PD4
	pinMode(PIN_BUZZER1, OUTPUT); 		// PC2 *
	pinMode(PIN_BUZZER2, OUTPUT); 		// PC3 *
}

void pins_off()
{
	// отключаем дисплей
	matrix.end();

	TIMSK0 &=~ (1 << OCIE0A);
	while(ASSR&(1<<TCN2UB)); //ждем пока отдуплится таймер
	SPCR = (0 << SPE);

	while (ADIF == 0); //Ждем флага окончания преобразования на случай если сон застал в процессе замера
	ADCSRA = (0 << ADEN); //Выключение АЦП

	beep_counter=0;

	// lcd, накачку, reset, питание дисплея, пищалку - переводим на входы, накачку отключаем
	pinMode(PIN_PUMP, OUTPUT);    		// PB0
	digitalWrite(PIN_PUMP, LOW);
	pinMode(PIN_BTN1, INPUT); 			// PD2
	pinMode(PIN_SENSOR, INPUT); 		// PD3
	pinMode(PIN_BTN2, INPUT); 			// PD4
	pinMode(PIN_HV, INPUT); 			// PD5
	pinMode(PIN_BUZZER1, INPUT); 		// PC2 *
	pinMode(PIN_BUZZER2, INPUT); 		// PC3 *

	digitalWrite(PIN_BTN1, LOW);
	digitalWrite(PIN_BTN2, LOW);
	digitalWrite(PIN_SENSOR, LOW);
	digitalWrite(PIN_HV, LOW);
	digitalWrite(PIN_BUZZER1, LOW);
	digitalWrite(PIN_BUZZER2, LOW);

	// lcd, накачка
	/*DDRB =  (0 << DDB2)|(0 << DDB3)|(0 << DDB4)|(0 << DDB5) | (1 << DDB0);
	PORTB = (0 << PORTB2)|(0 << PORTB3)|(0 << PORTB4)|(0 << PORTB5) | (0 << PORTB0);*/

	// reset, питание дисплея, пищалка
	/*PORTC = (0 << PORTC1) | (0 << PORTC0) | (0 << PORTC2)|(0 << PORTC3);
	DDRC = (0 << DDC1) | (0 << DDC0) | (0 << DDC2)|(0 << DDC3);*/

	// Датчик 0, капа 1
	/*DDRD |=  (0 << DDD3) | (0 << DDD2);
	PORTD |= (1 << PORTD3) | (1 << PORTD2);*/

	SMCR = (1 << SM1)|(1 << SM0)|(1 << SE); // Разрешаем сон
	PRR |= (1 << PRADC)|(1 << PRSPI)|(1 << PRTIM0); // отрубаем АЦП, СПИ, таймер писка
	EIMSK |= (1 << INT0); // wake button
	awaken=0;
}


//////////////////////////////////////////////////////////////////////////
void setup()
{
	//for (unsigned char f=0; f<237; f++) sbm[f]=0; // на всякий случай
	
	//mySerial.begin(9600);
	//mySerial.println(F("Nanit v3.0"));

	// накачка - выход, выкл
	pinMode(PIN_PUMP, OUTPUT);
	digitalWrite(PIN_PUMP, LOW);
	
	// Датчик, кнопка 1 - входы с подтяжками
	pinMode(PIN_SENSOR, INPUT);
	pinMode(PIN_BTN1, INPUT);

	// Включаем остальные пины
	pins_on();

	TCCR2B = 0x00;
	ASSR|=(1<<AS2); //ассинхронный режим

	TCNT2 = 0xE0; 			//presetup (-0x1F)
	OCR2A = 0x20;			// 32768/1024=32=0x20 for 1s.
	OCR2B = 0x00;
	TCCR2A = 0x00; 
	TCCR2B = 0x07; 			//start /1024
	TIMSK2 = 0x01; 			//timer2 ovf interrupt enable

	// счётчик для ОС с FlyBack сквозь T1 по спадающему фронту
	TCCR1A = 0x00; 
	TCCR1B |= (1<<CS12)|(1<<CS11);
	TCCR1B &=~ (1<<CS10);

	// Звук
	PRR &=~ (1<<PRTIM0)|(1<<PRADC);
	TCCR0A |= (1<<WGM01);			 // mode2: ctc
	TCCR0B |= (1<<CS01); // clock/8

	OCR0A=195;
	TCNT0=0;
  
	// питание дисплея

	// чтение настроек из eeprom
	EEPROM.get(EEPROM_ADDR_IMPULSE, impulse);
	EEPROM.get(EEPROM_ADDR_BEEP_LEVEL, beep_level);
	EEPROM.get(EEPROM_ADDR_ION, ion);
    EEPROM.get(EEPROM_ADDR_SLEEP_LEVEL, sleep_level);
	EEPROM.get(EEPROM_ADDR_ALARM_LEVEL, alarm_level);
	EEPROM.get(EEPROM_ADDR_COUNTER, counter);
	EEPROM.get(EEPROM_ADDR_BRIGHTNESS_LEVEL, brightness_level);

	#if (SENSOR_TYPE == SENSOR_M_SEL)	
		if (counter==SENSOR_SBM10) sbm_count_time=220;
		else sbm_count_time=420;
		count_validate = sbm_count_time;
	#endif

	// set defaults if broken EEPROM setup
	if (impulse == 255 || beep_level == 255 || ion == 255 || sleep_level == 255 || alarm_level == 255 || counter == 255 || brightness_level == 255) {
		impulse = 2; beep_level = 1; ion = 110; sleep_level = 30; alarm_level = 80; counter = SENSOR_SBM21; brightness_level = 3;
		EEPROM.put(EEPROM_ADDR_IMPULSE, impulse);
		EEPROM.put(EEPROM_ADDR_BEEP_LEVEL, beep_level);
		EEPROM.put(EEPROM_ADDR_ION, ion);
		EEPROM.put(EEPROM_ADDR_SLEEP_LEVEL, sleep_level);
		EEPROM.put(EEPROM_ADDR_ALARM_LEVEL, alarm_level);
		EEPROM.put(EEPROM_ADDR_COUNTER, counter);
		EEPROM.put(EEPROM_ADDR_BRIGHTNESS_LEVEL, brightness_level);
	}

	//check_battery();
	batt=1;

	EICRA = (0 << ISC00)|(0 << ISC01)| // по низкому уровню
			(0 << ISC10)|(1 << ISC11); // по спадающему фронту. По низкому уровню не работает в спящем режиме, т.е. не считает!!!

	EIMSK |= (1 << INT1);

	lcd_init();
	matrix.clear();
	matrix.writeDisplay();

	scr=state_boot;
	lcd_draw_screen();
	scr=state_main;
	_delay_ms(1000);
	lcd_clear();
	sei();
}

void loop()    
{
	if (go_shutdown)
	{
			go_shutdown=0;
			// Гасим всё. То есть совсем всё.
			matrix.end();
			EIMSK=0x01;  // wake button
			TIMSK0=0x00;
			TIMSK1=0x00;
			TIMSK2=0x00;
			PORTB=0x00;
			PORTC=0x00;
			PORTD=0x04;  // подтяжка wake button
			DDRB=0x00;
			DDRC=0x00;
			DDRD=0x00;
			SMCR = (1 << SE)|(0 << SM0)|(1 << SM1)|(0 << SM2); // Разрешаем выключение
			PRR |= (1 << PRADC)|(1 << PRSPI)|(1 << PRTWI)|(1 << PRTIM0); // отрубаем АЦП, СПИ, i2c, таймер писка
			do_shutdown=1;
			asm("sleep");
	}

	if (go_to_bed)	{ go_to_bed=0; pins_off();}

	if (out_of_bed)
	{
		out_of_bed=0;
		pins_on();
		EIMSK &=~ (1 << INT0);
		digitalWrite(PIN_PUMP, LOW);
		//digitalWrite(PIN_OLED_VCC, HIGH);
		/*PORTB &= ~(1<<PORTB0); //на всякий...
		PORTC |= (1<<PORTC0); // питание дисплея вкл*/
		//if (light_level!=2) PORTC |= (1<<PORTC1); // подсветка вкл
		//menu=state_menu_alarm;
		menu=state_menu_alarm;
		scr=state_main;
		displaying_dose=state_main_current;
		key=state_key_none;
		lcd_init();
		lcd_clear();
		lcd_redraw=1;
	}

	if (!do_shutdown)
	{ // POWER OFF		

		if (recount)
		{
			recount=0;
			count_rad();
		}

		if ((awaken)&&(lcd_redraw)) 
		{
			//lcd_redraw=0;
			lcd_draw_screen();
			//if (scr==state_main) lcd_draw_graph();
		}


		if ((batt)&&(awaken)) {batt=0; check_battery();}

		if (awaken) {check_key(); if (key != state_key_none) process_key();}
		if ((piip)&&(awaken)&&(beep_level)) {piip=0; beep(BEEP_TONE_PWR, 65);	}

	} // POWER OFF

	if ((!awaken)||(do_shutdown)) asm("sleep");
}
