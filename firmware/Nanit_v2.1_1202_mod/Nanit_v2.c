/*
 * nanit.c
 *
 * Created: 2014
 * Author: MadOrc
 * Optimization: DooMmen
 * 
 * ВЕРСИЯ 2.1 релиз
 */  

#include "Nanit_v2.h"

//////////////////////
void nlcd_Print_str(uint32_t message, unsigned char number_size, unsigned char leading_zero, unsigned char x, unsigned char y, unsigned char inv);
void Init_LCD();
void PinsOn();
void PinsOff();
void CheckKey();
void Draw_Screen();
void Count_Rad();
void Beep(unsigned char tone, unsigned char duration);
static void Pump();
void CheckBatt();

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
		do_shutdown=0;
		resetMC();
	}

	if (sleep_level) awaken=sleep_level+1; else awaken=61;
	if (light_level) light=light_level+1; else light=61;
	scr=0; // Если уснули в меню - проснемся на главном
	out_of_bed=1;
	piip=0;
	Beep(22, 20);
	key=5; // пустая обработка, чтобы при выходе из сна, не реагировал на кнопку
}		

//////////////////////////////////////////////////////////////////////////
ISR(INT1_vect) // импульсы с датчика 0
{
	PORTB &= ~(1<<PORTB0); //на всякий...
	sbm[0]++;
	piip=1;
}

//////////////////////////////////////////////////////////////////////////
ISR(TIMER2_OVF_vect)	// обработчик прерывания по совпадению Т2
{                       
	PORTB &= ~(1<<PORTB0);	//на всякий...

	TCNT2 = 0xE0;			//обновим счетчик

	second++;				// увеличиваем секунды

	if (awaken==1) go_to_bed=1; // СПАТЬ!
	if (light==1) {PORTC &=~ (1<<PORTC1); }// гасим свет
	
	if ((awaken)&&(sleep_level)) awaken--; //обратный отсчёт до сна
	if ((light)&&(light_level)) light--; //обратный отсчёт до вырубания подсветки


	if (count_validate>0) count_validate--;

//	if (sbm_count_time-count_validate > 3) // сброс валидатора, для оперативного пересчета показаний при резкой смене фона, сравним элементы массива импульсов через 1
//	{
//		if ((sbm[1]>sbm[3])&&((sbm[1]-sbm[3])>impulse_threshold)) count_validate=sbm_count_time;
//		if ((sbm[1]<sbm[3])&&((sbm[3]-sbm[1])>impulse_threshold)) count_validate=sbm_count_time;
//	}

	#if (COUNTER_D == SBM21)	// 3!! секунды
		if (!count_flag)	// 0
		{
			div_graph_sbm=1;
			for(unsigned char i=139;i>0;i--) // k - не менее отображаемого графа [48]/[96]!
			{
				sbm[i]=sbm[i-1];
				#ifdef G_2_LINE
					if ((sbm[i]>div_graph_sbm)&&(i<48)) div_graph_sbm=sbm[i]; // Делитель графиков.
				#else
					if ((sbm[i]>div_graph_sbm)&&(i<96)) div_graph_sbm=sbm[i]; // Делитель графиков.
				#endif
			}
			sbm[0]=0;
		}
		if (count_flag<2) count_flag++; else count_flag=0;

	#elif (COUNTER_D == SBM21_2) // 2!! секунды
		if (count_flag)
		{
			div_graph_sbm=1;
			for(unsigned char i=107;i>0;i--) // k - не менее отображаемого графа [48]/[96]!
			{
				sbm[i]=sbm[i-1];
				#ifdef G_2_LINE
					if ((sbm[i]>div_graph_sbm)&&(i<48)) div_graph_sbm=sbm[i]; // Делитель графиков.
				#else
					if ((sbm[i]>div_graph_sbm)&&(i<96)) div_graph_sbm=sbm[i]; // Делитель графиков.
				#endif
			}
			sbm[0]=0;
		}
		if (count_flag) count_flag=0; else count_flag=1;

	#elif (COUNTER_D == SI19) // 2 секунды
		if (count_flag)
		{
			div_graph_sbm=1;
			for(unsigned char i=50;i>0;i--) // k - не менее отображаемого графа [48]!
			{
				sbm[i]=sbm[i-1];
				if ((sbm[i]>div_graph_sbm)&&(i<48)) div_graph_sbm=sbm[i]; // Делитель графиков.
			}
			sbm[0]=0;
		}
		if (count_flag) count_flag=0; else count_flag=1;

	#elif (COUNTER_D == SBM10)   // 2 секунды
		if (count_flag)
		{
			div_graph_sbm=1;
			for(unsigned char i=109;i>0;i--) // k - не менее отображаемого графа [48]/[96]!
			{
				sbm[i]=sbm[i-1];
				#ifdef G_2_LINE
					if ((sbm[i]>div_graph_sbm)&&(i<48)) div_graph_sbm=sbm[i]; // Делитель графиков.
				#else
					if ((sbm[i]>div_graph_sbm)&&(i<96)) div_graph_sbm=sbm[i]; // Делитель графиков.
				#endif
			}
			sbm[0]=0;
		}
		if (count_flag) count_flag=0; else count_flag=1;

	#elif (COUNTER_D == SBM20) // 1!!! секунда
		div_graph_sbm=1;
		for(unsigned char i=50;i>0;i--) // k - не менее отображаемого графа [48]!
		{
			sbm[i]=sbm[i-1];
			if ((sbm[i]>div_graph_sbm)&&(i<48)) div_graph_sbm=sbm[i]; // Делитель графиков.
		}
		sbm[0]=0;

	#elif (COUNTER_D == M_SEL)
		if (counter == SBM10)
		{
			if (count_flag<1) count_flag++; else count_flag=0;
		}
		else
		{
			if (count_flag<2) count_flag++; else count_flag=0;
		}
		if (!count_flag)
		{
			div_graph_sbm=1;
			unsigned char m;
			if (counter == SBM10) m=109;
			else m=139;
			for(unsigned char i=m;i>0;i--) // k - не менее отображаемого графа [48]!
			{
				sbm[i]=sbm[i-1];
				if ((sbm[i]>div_graph_sbm)&&(i<96)) div_graph_sbm=sbm[i]; // Делитель графиков.
			}
		sbm[0]=0;
		}
	#else
	#error *** Не определен тип датчика/GM tube type not defined ***
	#endif

	//if ((rad>radmax)&&(!count_validate)) radmax=rad; // Валидно - если измерено.
    if (rad>radmax) radmax=rad; // Забиваем на валидность
	fonsecond += rad; // 
	fon_daily=(fon_daily+fonsecond)/3600;
	if(second == 60)			// если second = 60, second = 0
	{  // second
		second = 0;
		minute++;				// увеличиваем минуты
		if (alarm_snooze) alarm_snooze--;
		if (!rad) fonerr++;
		if ((fonerr>10)&&(!alarm_snooze)) { nlcd_Clear(); scr=5; alarm_beep=2; fonerr=0;} // Ошибка датчика, нет импульсов 
		batt=1;

		if(minute == 60)		// если minute = 60, minute = 0
		{  // minute
			minute = 0;
			hour++;				// увеличиваем часы

		if(hour == 24)			// если hour = 24, hour = 0
		{ // hour
			hour = 0;
			days++;
			fonall+=fon_daily;
			fonsecond=0;
			fon_daily=0;
			radmax=0;

		} // hour
		} // minute
		} // second

	if (second%2==0) Pump();
	redraw_LCD=1;
	recount=1;
}

//////////////////////////////////////////////////////////////////////////
ISR(TIMER0_COMPA_vect)
{ //sound
	if (blink_status) 
	{
		BEEP_1;
		blink_status=0;
	}
	else
	{
		BEEP_2;
		blink_status=1;
	}

	if (beep_counter++>beep_length)	// 
	{ 
		TIMSK0 &=~ (1 << OCIE0A); 
		BEEP_OFF;
		beep_counter=0; 
		if (alarm_beep==2)	{alarm_beep=0; Beep(20, 254);}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// пихает в *buffer строковый вид числа valu (не более 6 знаков), number_size - сколько нам надо разрядов (1-6), leading_zero: 0 - без ведущих нулей, 1 - с нулями
// unsigned char *int_to_str(char *buffer, uint32_t valu, unsigned char number_size, unsigned char leading_zero);
char *int_to_str(char *buffer, uint32_t valu, unsigned char number_size, unsigned char leading_zero)
{
	char *ptr = buffer;
	uint8_t i = (6-number_size);
	do
	{
		uint32_t pow6 = pow10Table32[i++];
		uint8_t count = 0;
		while(valu >= pow6)
		{
			count ++;
			valu -= pow6;
		}
		*ptr++ = count + '0';
	}while(i < 6);
	*ptr = 0;
	// удаляем ведущие нули
	if (!leading_zero) 
	{
		while(buffer[0] == '0') { if (buffer[1]) buffer[0] = ' '; ++buffer; }
	}
	else
	{
		while(buffer[0] == '0') ++buffer;	
	}
	return buffer;
}


//Оптимизированный вывод на дисплей
void nlcd_Print_str(uint32_t message, unsigned char number_size, unsigned char leading_zero, unsigned char x, unsigned char y, unsigned char inv)
{
					int_to_str(txt, message, number_size, leading_zero);
					nlcd_Print(txt, x, y, inv);
}


//Инициализация дисплея
void Init_LCD()
{
	nlcd_Init(contrast_level);
	nlcd_Clear();
	nlcd_Inverse(inverse_level);
	#ifdef MIRR
		nlcd_SendByte(CMD_LCD_MODE,0xA0);
	#else
		nlcd_SendByte(CMD_LCD_MODE,0xA1);
	#endif
}


void Beep(unsigned char tone, unsigned char duration)
{
	if (beep_counter) return;
	beep_length=duration*10;
	OCR0A=tone;
	TIMSK0 |= (1 << OCIE0A);
}


static void Pump()
{
	unsigned int pumpa;

	if (impulse>11) impulse=11;
	if (impulse<1) impulse=1;

	pumpbreak=0;

	pumpa=0;
	do
	{
		pumpbreak++;
		PORTB |= (1<<PORTB0);
		for (unsigned char r=0; r<impulse; r++) asm("nop");
		PORTB &= ~(1<<PORTB0);

		TCNT1=0;
		_delay_us(38);
		hv_status=TCNT1;

		if (hv_status) { PORTB &= ~(1<<PORTB0); pumpa=800; } // выходим //7
	}
	while (pumpa++<800);

	PORTB &= ~(1<<PORTB0);
}


void Count_Rad()
{
	volatile unsigned long fon=0;

	#if (COUNTER_D == SBM21)
		const unsigned int counter_base=1400; // время датчика / 3 * 10
		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 3(ТРИ) СЕКУНДЫ !!!
		for (unsigned char i=1; i<140; i++)
		{
			fon+=sbm[i];

			if (i==5)
			{
				if(fon>1800) // фон больше 46800 мкр/ч интервал расчета 15 сек
				{
					fon*=28;
					if (sbm_count_time!=15) count_validate=15;
					sbm_count_time=15;
					i=140; //вылет из цикла, экономим такты
				}
			}
	
			if (i==28)
			{
				if(fon>200) //  фон больше 1000 мкр/ч интервал расчета 84 сек
				{
					fon*=5;
					if (sbm_count_time!=84) count_validate=84;
					sbm_count_time=84;
					i=140; //вылет из цикла, экономим такты
				}
			}

			if (i==139) 
			{
				if (sbm_count_time!=420) count_validate=420;
				sbm_count_time=420;
			}
		}

	#elif (COUNTER_D == SBM21_2)
		const unsigned int counter_base=1080; // время датчика /2 * 10
		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 2(две) СЕКУНДЫ!!!
		for (unsigned char i=1; i<108; i++)
		{
			fon+=sbm[i];

			if (i==12)
			{
				if(fon>1800) //фон больше 16 200 мкр/ч интервал расчета 24 сек
				{
					fon*=9; //
					if (sbm_count_time!=24) count_validate=24;
					sbm_count_time=24;
					i=108; //вылет из цикла, экономим такты
				}
			}

			if (i==18)
			{
				if(fon>200) // фон больше 1200 мкр/ч  интервал расчета 36 сек
				{
					fon*=6;
					if (sbm_count_time!=36) count_validate=36;
					sbm_count_time=36;
					i=108; //вылет из цикла, экономим такты
				}
			}

			if (i==54)
			{
				if(fon>100)// фон больше 200 мкр/ч  интервал расчета 108 сек
				{
					fon*=2;
					if (sbm_count_time!=108) count_validate=108;
					sbm_count_time=108;
					i=108; //вылет из цикла, экономим такты
				}
			}

			if (i==107)
			{
				if (sbm_count_time!=216) count_validate=216;
				sbm_count_time=216;
			}
		}

	#elif (COUNTER_D == SI19)
		const unsigned int counter_base=320; // время датчика /2 * 10
		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 2(две) СЕКУНДЫ
		for (unsigned char i=1; i<32; i++)
		{
			fon+=sbm[i];

			if (i==2)
			{
				if(fon>700) // фон больше 14000 мкр/ч интервал расчета 4 сек
				{
					fon*=20;
					if (sbm_count_time!=4) count_validate=4;
					sbm_count_time=4;
					i=32; //вылет из цикла, экономим такты
				}
			}

			if (i==8)
			{
				if(fon>300) // фон больше 1200 мкр/ч интервал расчета 16 сек
				{
					fon*=4;
					if (sbm_count_time!=16) count_validate=16;
					sbm_count_time=16;
					i=32; //вылет из цикла, экономим такты
				}
			}

			if (i==16)
			{
				if(fon>50) // фон больше 100 мкр/ч интервал расчета 32 сек
				{
					fon*=2;
					if (sbm_count_time!=32) count_validate=32;
					sbm_count_time=32;
					i=32; //вылет из цикла, экономим такты
				}
			}

			if (i==31)
			{
				if (sbm_count_time!=64) count_validate=64;
				sbm_count_time=64;
			}
		}

	#elif (COUNTER_D == SBM20)
		const unsigned int counter_base=440; // время датчика /2 * 10
		// СБМ-20 датчик без коррекции жёсткости хода (голый) СБМ20 - 44 сек
		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 2(две) СЕКУНДЫ
		for (unsigned char i=1; i<44; i++)
		{
			fon+=sbm[i];

			if (i==6)
			{
				if(fon>2000)// фон больше 5500 мкР.ч интервал расчета 4 сек
				{
					fon*=11;
					if (sbm_count_time!=6) count_validate=6;
					sbm_count_time=6;
					i=44; //вылет из цикла, экономим такты
				}
			}

			if (i==22)
			{
				if(fon>100) //  фон больше 100 мкР.ч интервал расчета 22 сек
				{
					fon*=2;
					if (sbm_count_time!=22) count_validate=22;
					sbm_count_time=22;
					i=44; //вылет из цикла, экономим такты
				}
			}

			if (i==43)
			{
				if (sbm_count_time!=44) count_validate=44;
				sbm_count_time=44;
			}
		}

	#elif (COUNTER_D == SBM10)
		const unsigned int counter_base=1100; // время датчика / 2 * 10
		// СБМ-10 После долгих прикидок и раздумий - датчик без коррекции жёсткости хода (голый) СБМ10 - 220с [78(чувствительность известного нам датчика сбм-20)/12(чувствительность датчика сбм-10)*34(время счёта СБМ20 так все-таки правильнее)]
		// если будем корректировать к примеру свинцовой фольгой чувствительность к частицам низких энергий время будет другим

		// В ОДНОЙ ЯЧЕЙКЕ SBM[] 2(две) СЕКУНДЫ 220
		for (unsigned char i=1; i<110; i++)
		{
			fon+=sbm[i];

			if (i==10)
			{
				if(fon>18181) // фон больше 200 000 мкр/ч интервал расчета 20 сек
				{
					fon*=11; //
					if (sbm_count_time!=20) count_validate=20;
					sbm_count_time=20;
					i=110; //вылет из цикла, экономим такты
				}
			}

			if (i==28)
			{
				if(fon>20000) // фон больше 80 000 мкр/ч интервал расчета 55 сек
				{
					fon*=4;
					if (sbm_count_time!=55) count_validate=55;
					sbm_count_time=55;
					i=110; //вылет из цикла, экономим такты
				}
			}

			if (i==55)
			{
				if(fon>250) // фон больше 500 мкр/ч интервал расчета 110 сек
				{
					fon*=2;
					if (sbm_count_time!=110) count_validate=110;
					sbm_count_time=110;
					i=110; //вылет из цикла, экономим такты
				}
			}

			if (i==109)
			{
				if (sbm_count_time!=220) count_validate=220;
				sbm_count_time=220;
			}
		}

	#elif (COUNTER_D == M_SEL)
		if (counter==SBM10)
		{	// SBM10
			for (unsigned char i=1; i<110; i++)
			{
				fon+=sbm[i];

				if (i==10)
				{
					if(fon>18181) // фон больше 200 000 мкр/ч интервал расчета 20 сек
					{
						fon*=11; //
						if (sbm_count_time!=20) count_validate=20;
						sbm_count_time=20;
						i=110; //вылет из цикла, экономим такты
					}
				}

				if (i==28)
				{
					if(fon>20000) // фон больше 80 000 мкр/ч интервал расчета 55 сек
					{
						fon*=4;
						if (sbm_count_time!=55) count_validate=55;
						sbm_count_time=55;
						i=110; //вылет из цикла, экономим такты
					}
				}

				if (i==55)
				{
					if(fon>250) // фон больше 500 мкр/ч интервал расчета 110 сек
					{
						fon*=2;
						if (sbm_count_time!=110) count_validate=110;
						sbm_count_time=110;
						i=110; //вылет из цикла, экономим такты
					}
				}

				if (i==109)
				{
					if (sbm_count_time!=220) count_validate=220;
					sbm_count_time=220;
				}
			}
		}
		else
		{	// SBM21
			for (unsigned char i=1; i<140; i++)
			{
				fon+=sbm[i];
				if (i==5)
				{
					if(fon>1800) // фон больше 46800 мкр/ч интервал расчета 15 сек
					{
						fon*=28;
						if (sbm_count_time!=15) count_validate=15;
						sbm_count_time=15;
						i=140; //вылет из цикла, экономим такты
					}
				}
	
				if (i==28)
				{
					if(fon>200) //  фон больше 1000 мкр/ч интервал расчета 84 сек
					{
						fon*=5;
						if (sbm_count_time!=84) count_validate=84;
						sbm_count_time=84;
						i=140; //вылет из цикла, экономим такты
					}
				}

				if (i==139) 
				{
					if (sbm_count_time!=420) count_validate=420;
					sbm_count_time=420;
				}
			}
		}
	#else
	#error *** Не определен тип датчика/GM tube type not defined ***
	#endif

	#if (COUNTER_D == SBM21)  // 3 секунды !!!
		if (count_validate && (sbm_count_time-count_validate>4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
		{
			unsigned char approx=(sbm_count_time-count_validate)/3;
			fon=0;
			for (unsigned char i=1; i<approx+3;i++)
			{
				fon = fon + sbm[i];
			}
			fon=fon*counter_base/approx/10; // *на 10 из-за 8битной целой математики, фигово тут сплавающей точкой, лучше даже не связываться, просто "сдвигаем" 10е доли
		}

	#elif (COUNTER_D == SBM20)  // 1 секунда !!!
		if (count_validate && (sbm_count_time-count_validate>4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
		{
			unsigned char approx=(sbm_count_time-count_validate);
			fon=0;
			for (unsigned char i=1; i<approx+1;i++)
			{
				fon = fon + sbm[i];
			}
			fon=fon*counter_base/approx/10; // *на 10 из-за 8битной целой математики, фигово тут сплавающей точкой, лучше даже не связываться, просто "сдвигаем" 10е доли
		}

	#elif (COUNTER_D == M_SEL)
		unsigned char t;
		unsigned int counter_base;
		if (counter == SBM10){
			counter_base=1100;
			t=2;}
		else{
			counter_base=1400;
			t=3;}
		if (count_validate && (sbm_count_time-count_validate>4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
		{
			unsigned char approx=(sbm_count_time-count_validate)/t;
			fon=0;
			for (unsigned char i=1; i<approx+t;i++)
			{
				fon = fon + sbm[i];
			}
			fon=fon*counter_base/approx/10;	// *на 10 из-за 8битной целой математики, фигово тут сплавающей точкой, лучше даже не связываться, просто "сдвигаем" 10е доли
		}

	#else // 2 секунды
		if (count_validate && (sbm_count_time-count_validate>4)) // не 0 значит не обсчитано, значит моргаем прикидочным значением, больше секунд прошло - точнее
		{
			unsigned char approx=(sbm_count_time-count_validate)/2;
			fon=0;
			for (unsigned char i=1; i<approx+2;i++)
			{
				fon = fon + sbm[i];
			}
			fon=fon*counter_base/approx/10; // *на 10 из-за 8битной целой математики, фигово тут сплавающей точкой, лучше даже не связываться, просто "сдвигаем" 10е доли
		}
	#endif

	rad=fon;

	#ifdef DOSE
		if ((fon_daily/1000>alarm_level)&&(!alarm_snooze)&&(alarm_level))
	#else
		if ((rad>alarm_level)&&(!alarm_snooze)&&(alarm_level)&&(sbm_count_time-count_validate>5))
	#endif
	{
		if (!awaken) // если спим - ПОДЪЁМ!
		{
			SMCR &=~ (1 << SE); // сны запрещены
			awaken=61;
			light=light_level+1;
		   	PinsOn();
		   	EIMSK &=~ (1 << INT0);
		   	PORTB &= ~(1<<PORTB0); //на всякий...
		   	PORTC |= (1<<PORTC0); // питание дисплея вкл
		   	if (light_level!=2) PORTC |= (1<<PORTC1); // подсветка вкл
		   	punkt=1;

			Init_LCD();
		}

		alarm_beep=2; Beep(20, 254);
		if (scr!=3) nlcd_Clear();
			scr=3;
		redraw_LCD=1;
	}
}


void Draw_Graph()
{
	unsigned char a, val, line_lo, line_hi;
	#ifndef  OLD_DRAW
		unsigned char line_mid;
	#endif

	// битовыми сдвигами лучше, спасибо Shodan за подсказку ;)
	unsigned char  a_cnt=0;

	#ifndef G_2_LINE
		for (a=0; a < 96; a++)
	#else
		for (a=0; a < 48; a++)
	#endif

	#ifdef OLD_DRAW
		{
			line_lo=0xff;
			line_hi=0xff;

			val=(sbm[a]*14/div_graph_sbm);

			val=14-val;

			line_hi=line_hi<<(val+2);
			if (val>8) line_lo=line_lo<<(val-8);


			line_lo&=0x7f; // убираем лишнюю линию

			nlcd_GotoXY(a_cnt,5);
			nlcd_SendByte(DATA_LCD_MODE,line_lo);

			nlcd_GotoXY(a_cnt,4);
			nlcd_SendByte(DATA_LCD_MODE,line_hi);

			a_cnt++;

			#ifdef G_2_LINE
				nlcd_GotoXY(a_cnt,5);
				nlcd_SendByte(DATA_LCD_MODE,line_lo);

				nlcd_GotoXY(a_cnt,4);
				nlcd_SendByte(DATA_LCD_MODE,line_hi);

				a_cnt++;

			#endif
		}
	#else
		{
			line_lo=0xff;
			line_mid=0xff;
			line_hi=0xff;

			val=(sbm[a]*22/div_graph_sbm);

			val=22-val;

			line_hi=line_hi<<(val+2);
			if (val>8) line_mid=line_mid<<(val-8);
			if (val>16) line_lo=line_lo<<(val-16);

			line_lo&=0x7f; // убираем лишнюю линию

			nlcd_GotoXY(a_cnt,6);
			nlcd_SendByte(DATA_LCD_MODE,line_lo);

			nlcd_GotoXY(a_cnt,5);
			nlcd_SendByte(DATA_LCD_MODE,line_mid);

			nlcd_GotoXY(a_cnt,4);
			nlcd_SendByte(DATA_LCD_MODE,line_hi);

			a_cnt++;

			#ifdef G_2_LINE
				nlcd_GotoXY(a_cnt,6);
				nlcd_SendByte(DATA_LCD_MODE,line_lo);

				nlcd_GotoXY(a_cnt,5);
				nlcd_SendByte(DATA_LCD_MODE,line_mid);

				nlcd_GotoXY(a_cnt,4);
				nlcd_SendByte(DATA_LCD_MODE,line_hi);

				a_cnt++;
			#endif
		}
	#endif
}


void Draw_Screen()
{
	switch(scr)
	{
		case 0: // главный
		default:

			nlcd_Print("час", 74, 2, 0);
            // если текущий режим - максимальный уровень, выводим его вместо текущего
            #ifdef MAX_RAD_ON_MAIN
				    int_to_str(txt, radmax, 5, 0);
				    nlcd_Print("мкР", 74, 1, 0);
			        nlcd_PrintBig(txt, 5, 1);
				    nlcd_Print("Макс. доза", 4, 3, 0);
            // иначе - текущий фон показываем
            #else
			    if (rad<100000) 
			    {
				    int_to_str(txt, rad, 5, 0);
				    nlcd_Print("мкР", 74, 1, 0);
			    }
			    else 
			    {
				    int_to_str(txt, ((rad+500)/1000), 5, 0);
				    nlcd_Print(" мР", 74, 1, 0); 
			    }
			    nlcd_PrintBig(txt, 5, 1);	

			    nlcd_Print("(000/000с.)", 28, 3, 0);
			    if ((count_validate)&&(second%2==0))
			    {
				    nlcd_Print("Счет", 4, 3, 1);
			    }
			    else
			    {
				    nlcd_Print("Счет", 4, 3, 0);
			    }
			    nlcd_Print_str(sbm_count_time-count_validate, 3, 0, 34, 3, 0);
			    nlcd_Print_str(sbm_count_time, 3, 0, 58, 3, 0);
            #endif

			unsigned long fonad;	//для расчета fonall+fon_daily

			#ifdef OLD_DRAW
				nlcd_Print("00:00", 4, 7, 0);
				nlcd_Print_str(hour, 2, 1, 4, 7, 0);
				nlcd_Print_str(minute, 2, 1, 22, 7, 0);

				nlcd_Print("#100%$", 58, 7, 0);
				nlcd_Print_str(battery_percent, 3, 0, 64, 7, 0);


				switch (displaying_dose)
				{ //dd
					case 0:
					default:

                        #ifdef MAX_RAD_ON_MAIN
						    nlcd_Print("Сейчас ", 4, 6, 0);
						    if (rad<1000)
						    {
							    nlcd_Print("000мкР/ч", 46, 6, 0);
							    nlcd_Print_str(rad, 3, 0, 46, 6, 0);
						    }
						    else
						    {
							    nlcd_Print("000 мР/ч", 46, 6, 0);
							    nlcd_Print_str((rad+500)/1000, 3, 0, 46, 6, 0);
						    }
                        #else
						    nlcd_Print("Макс.  ", 4, 6, 0);
						    if (radmax<1000)
						    {
							    nlcd_Print("000мкР/ч", 46, 6, 0);
							    nlcd_Print_str(radmax, 3, 0, 46, 6, 0);
						    }
						    else
						    {
							    nlcd_Print("000 мР/ч", 46, 6, 0);
							    nlcd_Print_str((radmax+500)/1000, 3, 0, 46, 6, 0);
						    }
                        #endif;

						break;

					case 1:
						nlcd_Print("Сегодня ", 4, 6, 0);
						if (fon_daily<10000)
						{
							nlcd_Print("0000мкР", 52, 6, 0);
							nlcd_Print_str(fon_daily, 4, 0, 52, 6, 0);
						}
						else
						{
							nlcd_Print("0000 мР", 52, 6, 0);
							nlcd_Print_str((fon_daily+500)/1000, 4, 0, 52, 6, 0);
						}
						break;

					case 2:
						fonad = fonall+fon_daily;
						if (fonad<10000)
						{
							nlcd_Print("За 000д.0000мкР", 4, 6, 0);
						}
						else
						{
							nlcd_Print("За 000д.0000 мР", 4, 6, 0);
							fonad=(fonad+500)/1000;
						}
						nlcd_Print_str(days, 3, 0, 21, 6, 0);
						nlcd_Print_str(fonad, 4, 0, 52, 6, 0);
						break;
				} // dd
				break;
			#else
				switch (displaying_dose)

				{ //dd

					case 0:
					default:

						nlcd_Print("00:00:00 ",4,7,0);



						nlcd_Print_str(hour, 2, 1, 4, 7, 0);

						nlcd_Print_str(minute, 2, 1, 22, 7, 0);

						nlcd_Print_str(second, 2, 1, 40, 7, 0);



						nlcd_Print("#100%$",58,7,0);

						nlcd_Print_str(battery_percent, 3, 0, 64, 7, 0);

						break;



					case 1:
						nlcd_Print("Макс.  ",4,7,0);

						if (radmax<1000)

						{

							nlcd_Print("000мкР/ч",46,7,0);

							nlcd_Print_str(radmax, 3, 0, 46, 7, 0);

						}

						else

						{

							nlcd_Print("000 мР/ч",46,7,0);

							nlcd_Print_str((radmax+500)/1000, 3, 0, 46, 7, 0);

						}

						break;


					case 2:
						nlcd_Print("Сегодня ",4,7,0);

						if (fon_daily<10000)

						{

							nlcd_Print("0000мкР",52,7,0);

							nlcd_Print_str(fon_daily, 4, 0, 52, 7, 0);

						}

						else

						{

							nlcd_Print("0000 мР",52,7,0);

							nlcd_Print_str((fon_daily+500)/1000, 4, 0, 52, 7, 0);

						}

						break;

					case 3:
						fonad = fonall+fon_daily;
						if (fonad<10000)
						{
							nlcd_Print("За 000д.0000мкР", 4, 7, 0);
						}
						else
						{
							nlcd_Print("За 000д.0000 мР", 4, 7, 0);
							fonad=(fonad+500)/1000;
						}
						nlcd_Print_str(days, 3, 0, 21, 7, 0);
						nlcd_Print_str(fonad, 4, 0, 52, 7, 0);
						break;

				} // dd

				break;
			#endif

		case 1: // юзер меню

			nlcd_Print("( МЕНЮ )", 20, 0, 1); 

			if (alarm_level) 
			{
				#ifdef DOSE
					nlcd_Print("За день  00 мР",4,1,(punkt==1));
					nlcd_Print_str(alarm_level, 4, 0, 46, 1, (punkt==1));
				#else
					nlcd_Print("Трев.0000мкР/ч",4,1,(punkt==1)); 
					nlcd_Print_str(alarm_level, 4, 0, 34, 1, (punkt==1));
				#endif
		
		
			}
			else 
			{
				nlcd_Print("Тревога ВЫКЛ. ",4,1,(punkt==1)); 	
			}


			if (light_level==2)
			{
				nlcd_Print("Подсв. ВЫКЛ.  ",4,2,(punkt==2));
			}
			
			else if (light_level)
			{
				nlcd_Print("Подсв. 000 сек",4,2,(punkt==2));
				nlcd_Print_str(light_level, 3, 0, 46, 2, (punkt==2));
			}
			else
			{
				nlcd_Print("Подсв.  ВКЛ.  ",4,2,(punkt==2));
			}
	

			if (sleep_level)
			{
				nlcd_Print("Сон 000 сек   ",4,3,(punkt==3));
				nlcd_Print_str(sleep_level, 3, 0, 28, 3, (punkt==3));
			}
			else
				nlcd_Print("Сон запрещен  ",4,3,(punkt==3));

			if (beep_level)
				nlcd_Print("Импульс Звук  ",4,4,(punkt==4));
			else
				nlcd_Print("Импульс Тихо  ",4,4,(punkt==4));
				nlcd_Print("НАСТРОЙКИ &&&&",4,5,(punkt==5));
				nlcd_Print("Обнулить дозу!",4,6,(punkt==6));
				nlcd_Print("Выключить     ",4,7,(punkt==7));
			break;

		case 2: // настройки
			nlcd_Print(" НАСТРОЙКИ ",10,0,1);


			nlcd_Print("Ур.накачки:(000)",0,1,0);
			nlcd_Print_str(pumpbreak, 3, 0, 72, 1, 0);


			nlcd_Print("Часы: 00:00:00", 4,2,0);
			nlcd_Print_str(hour, 2, 0, 40, 2, (punkt==1));
			nlcd_Print_str(minute, 2, 1, 58, 2, (punkt==2));
			nlcd_Print_str(second, 2, 1, 76, 2, 0);


			nlcd_Print("Инверсно      ", 4, 3, (punkt==3));


			nlcd_Print("Контраст: 00  ",4,4,(punkt==4));
			nlcd_Print_str((contrast_level-128), 2, 0, 64, 4, (punkt==4));


			nlcd_Print("ИОН 000: 0.00В",4,5,(punkt==5));
			nlcd_Print_str(ion, 3, 0, 28, 5, (punkt==5));
			nlcd_Print_str((VoltLevel/100), 1, 0, 58, 5, (punkt==5));
			nlcd_Print_str((VoltLevel%100), 2, 1, 70, 5, (punkt==5));


			nlcd_Print("Накачка(2):00 ",4,6,(punkt==6));
			nlcd_Print_str(impulse, 2, 0, 70, 6, (punkt==6));


			#if (COUNTER_D == M_SEL)
				nlcd_Print("Датчик:  СБМ21",4,7,(punkt==7));
				if (counter==SBM10)
					nlcd_Print("10",76,7,(punkt==7));
			#endif

			break;
			
		case 3:
			#ifdef DOSE
				nlcd_Print("ВНИМАНИЕ!",21,2,0);
				nlcd_Print("Доза более",18,4,0);
				nlcd_Print("000000 Мр!",9,5,0);
				nlcd_Print_str(alarm_level, 6, 0, 9, 5, 0);
			break;
			#else
				nlcd_Print("ТРЕВОГА!",24,2,0);
				nlcd_Print("Фон более",21,4,0);
				nlcd_Print("000000 мкР/ч!",9,5,0);
				nlcd_Print_str(alarm_level, 6, 0, 9, 5, 0);
			#endif	
			break; 

		case 4:
		#ifdef LI_ION
			nlcd_Print("Зарядите",24,3,0);
			nlcd_Print("батарею!",24,4,0);
		#else
			nlcd_Print("Замените",24,3,0);
			nlcd_Print("батарейки!",18,4,0);
		#endif

		break;
		case 5:
			nlcd_Print("Ошибка датчика",6,3,0);
			nlcd_Print("нет импульсов!",6,4,0);
		break;

		case 10:
//			nlcd_Print("Гамма/Бета",18,1,0);
//			nlcd_Print("ДОЗИМЕТР",24,2,0);
//			nlcd_Print("НАНИТ",33,4,0);
//			nlcd_Print("MadOrc@gmx.com",6,5,0);
//			nlcd_Print("Прошивка 2.1",12,6,0);

			nlcd_Print("НАНИТ",33,4,0);
			nlcd_Print("Прошивка 2.1",12,6,0);

		#if (COUNTER_D == SBM21)
			nlcd_Print("Датчик СБМ-21",9,7,0);
		#elif (COUNTER_D == SBM21_2)
			nlcd_Print("2 Датчика СБМ-21",5,7,0);
		#elif (COUNTER_D == SI19)
			nlcd_Print("Датчик СИ-19ГМ",6,7,0);
		#elif (COUNTER_D == SBM20)
			nlcd_Print("Датчик СБМ-20",9,7,0);
		#elif (COUNTER_D == SBM10)
			nlcd_Print("Датчик СБМ-10",9,7,0);
		#elif (COUNTER_D == M_SEL)
			nlcd_Print("Датчик СБМ-21",9,7,0);
			if (counter==SBM10)
				nlcd_Print("10",75,7,0);
		#else
			#error *** Не определен тип датчика/GM tube type not defined ***
		#endif		
	}
}


void CheckKey()
{
	if (key) return; // пока капу не обработали, новых не читаем

	if((PIND&(1 << PORTD2))==0 )// если кнопка 1 нажата
	{
		key=1;
		Beep(22, 20);
	}
	else if((PIND&(1 << PORTD1))==0)// если кнопка 2 нажата
	{
		key=2;
		Beep(18, 20);
	}
}


void ReactKey()
{
	if (sleep_level) awaken=sleep_level+1; else awaken=61; 

	if (key==1)					// включение подсветки кнопкой без переключения
		if ((light<2) && (light_level!=2)) key=5;

	if (light_level) light=light_level+1; else light=61;

	if (light_level!=2) PORTC |= (1<<PORTC1); // подсветка вкл
	
	switch (scr)
	{
		case 0: // Мы на главном
			switch(key)
			{
				case 1:
				// вкл подсветки, обнуление сна
				displaying_dose++;
				#ifdef OLD_DRAW
					if (displaying_dose>2) displaying_dose=0; 
				#else
					if (displaying_dose>3) displaying_dose=0; 
				#endif
				break;
				case 2: // в меню
				scr=1;
				nlcd_Clear();
				redraw_LCD=1;
				break;
			}
			break;

		case 1: // Мы в меню
			switch(key)
			{
				case 1:
					// меняем чаво нада
					switch (punkt)
					{
						case 1: //тревога
							switch (alarm_level)
							{
								#ifdef DOSE
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
									case 500:
										alarm_level=1000;
										break;
									case 1000:
										alarm_level=3000;
										break;
								#endif
									default: // если 3000 или чтото не то -> 0
										alarm_level=0;
										break;
							}
							break;

						case 2: //подсветка
							switch (light_level)
							{
								case 0:
									light_level=2;
									break;
								case 2:
									light_level=10;
									break;
								case 10:
									light_level=30;
									break;
								case 30:
									light_level=60;
									break;
								default:
									light_level=0;
									break;
							}
							break;

						case 3: //сон 
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

						case 4: //имп звук
							if (beep_level) beep_level=0;
							else beep_level=1;
							break;

						case 5: //Перейти в настройки
							scr=2;
							punkt=1;
							nlcd_Clear();
							break;

						case 6: //Сброс дозы
							radmax=0;
							fonsecond=0;
							fon_daily=0;
							fonall=0;
							days=0;
							punkt=1;
							scr=0;
							nlcd_Clear();
							break;
						
						case 7: //Выключение
							go_shutdown=1;
							break;
						}
						break;
					case 2: // скачем по менюшкам
						if (punkt <7)	punkt++;
						else
						{ // выходим
							cli();
							eeprom_update_byte(&e_light_level, light_level);
							eeprom_update_byte(&e_beep_level, beep_level);
							eeprom_update_byte(&e_sleep_level, sleep_level);
							eeprom_update_word(&e_alarm_level, alarm_level);
							sei();
							scr=0;
							punkt=1;
							nlcd_Clear();
						}
						break;
					}
					redraw_LCD=1;
					break;
				case 2: // Мы в настройках
		switch(key)
		{
			case 1:
				switch (punkt)
				{
					case 1: //Часы
						if (hour<23) hour++; else hour=0;
						second=0;
						break;

						case 2: //Минуты
						if (minute<59) minute++; else minute=0;
						second=0;
						break;

						case 3: //Инверсия
						if (inverse_level) inverse_level=0; else inverse_level=1;
						nlcd_Inverse(inverse_level);
						break;

						case 4: //Контраст
						if (contrast_level<0x9F) contrast_level++; else contrast_level=0x80;
						nlcd_SendByte(CMD_LCD_MODE,contrast_level); //
						second=0;
						break;

						case 5: //Подстройка вольтметра
						if ((ion<124)&&(ion>99)) ion++; else ion=100;
						batt=1;
						break;

						case 6: //Подстройка накачки
						if ((impulse<11)&&(impulse>0)) impulse++; else impulse=1;
						break;

						#if (COUNTER_D == M_SEL)
							case 7: //Выбор датчика
							if (counter == SBM10){
								counter = SBM21;
								sbm_count_time=420;}
							else{
								counter = SBM10;
								sbm_count_time=220;}
							count_validate = sbm_count_time;
							break;
						#endif
				}

			break;
			case 2: // скачем по менюшкам
			#if (COUNTER_D == M_SEL)
				if (punkt <7)	punkt++;
			#else
				if (punkt <6)	punkt++;
			#endif
			else { // выходим
					cli();
						eeprom_update_byte(&e_ion, ion);
						eeprom_update_byte(&e_inverse_level, inverse_level);
						eeprom_update_byte(&e_contrast_level, contrast_level);
						eeprom_update_byte(&e_impulse, impulse);
						#if (COUNTER_D == M_SEL)
							eeprom_update_byte(&e_counter, counter);
						#endif
					sei();
					scr=0;
					punkt=1;
					nlcd_Clear();
				}
			break;
		}
		redraw_LCD=1;
		break;
		
		case 3: // Алярма
		case 4: // БатарЭя, Азиз
		case 5: // Нет импульсов
		case 6: // Недостаточное напряжение на датчике
			scr=0;
			nlcd_Clear();
			// alarm=0; 
			alarm_snooze=alarm_snooze_time; // сколько минут ни о чём не алармить
			redraw_LCD=1;
			break;
	}
	_delay_ms(400);
	key=0;
}


void CheckBatt()
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

	if ((VLcc<batt_warn)&&(!alarm_snooze)) { nlcd_Clear(); alarm_beep=2; scr=4; } // при замере юзер увидит, нефиг расходовать на "биби" и свет остатки батареи

	#ifdef LI_ION
		if (VLcc<320) VLcc=320;
		if (VLcc>420) VLcc=420;
		battery_percent=(VLcc-320);//*100/100//*183/256;//100/140;
	#else
		if (VLcc<245) VLcc=245;
		if (VLcc>300) VLcc=300;
		battery_percent=(VLcc-245)*100/55;
	#endif
}


void PinsOn()
{
	PRR = (0<<PRTIM0)|(0<<PRADC)|(0<<PRSPI);

		// подсветка, питание дисплея, пищалка
	DDRC |=  (1 << DDC1) | (1 << DDC0) | (1 << DDC2)|(1 << DDC3);
	PORTC |= (0 << PORTC1) | (0 << PORTC0) | (0 << PORTC2)|(0 << PORTC3);

	DDRD |=  (0 << DDD1);
	PORTD |= (1 << PORTD1);
}


void PinsOff()
{
	nlcd_SendByte(CMD_LCD_MODE,0xAE);

	TIMSK0 &=~ (1 << OCIE0A);
	while(ASSR&(1<<TCN2UB)); //ждем пока отдуплится таймер
	while(!(SPSR & (1<<SPIF))); // Ждём, пока кончит апп. SPI
	SPCR = (0 << SPE);

	while (ADIF == 0); //Ждем флага окончания преобразования на случай если сон застал в процессе замера
	ADCSRA = (0 << ADEN); //Выключение АЦП

		 // lcd, накачка
	DDRB =  (0 << DDB2)|(0 << DDB3)|(0 << DDB4)|(0 << DDB5) | (1 << DDB0);
	PORTB = (0 << PORTB2)|(0 << PORTB3)|(0 << PORTB4)|(0 << PORTB5) | (0 << PORTB0);

		// подсветка, питание дисплея, пищалка
	PORTC = (0 << PORTC1) | (0 << PORTC0) | (0 << PORTC2)|(0 << PORTC3);
	DDRC = (0 << DDC1) | (0 << DDC0) | (0 << DDC2)|(0 << DDC3);

	beep_counter=0;

		// Датчик 0, капа 1
	DDRD |=  (0 << DDD3) | (0 << DDD2);
	PORTD |= (1 << PORTD3) | (1 << PORTD2);

	SMCR = (1 << SM1)|(1 << SM0)|(1 << SE); // Разрешаем сон
	PRR |= (1 << PRADC)|(1 << PRSPI)|(1 << PRTIM0); // отрубаем АЦП, СПИ, таймер писка
	EIMSK |= (1 << INT0); // wake button
	awaken=0;
}


//////////////////////////////////////////////////////////////////////////
OS_MAIN int main(void)
{
	//for (unsigned char f=0; f<237; f++) sbm[f]=0; // на всякий случай
		// накачка
	DDRB |=  (1 << DDB0);
	PORTB |= (0 << PORTB0);
	
		// Датчик 0, капа 1
	DDRD |=  (0 << DDD3) | (0 << DDD2);
	PORTD |= (1 << PORTD3) | (1 << PORTD2);

	PinsOn();

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


		///Звук
	PRR &=~ (1<<PRTIM0)|(1<<PRADC);
	TCCR0A |= (1<<WGM01);
	TCCR0B |= (1<<CS00) | (1<<CS01);

	OCR0A=195;
	TCNT0=0;
  
  
	PORTC |= (1<<PORTC0); // питание дисплея вкл

	impulse=eeprom_read_byte(&e_impulse);
	inverse_level=eeprom_read_byte(&e_inverse_level);
	light_level=eeprom_read_byte(&e_light_level);
	beep_level=eeprom_read_byte(&e_beep_level);
	ion=eeprom_read_byte(&e_ion);
	sleep_level=eeprom_read_byte(&e_sleep_level);
	contrast_level=eeprom_read_byte(&e_contrast_level);
	alarm_level=eeprom_read_word(&e_alarm_level);
	#if (COUNTER_D == M_SEL)
		counter=eeprom_read_byte(&e_counter);
		if (counter==SBM10) sbm_count_time=220;
		else sbm_count_time=420;
		count_validate = sbm_count_time;
	#endif

	//CheckBatt();
	batt=1;

	EICRA = (0 << ISC00)|(0 << ISC01)| // по низкому уровню
			(0 << ISC10)|(1 << ISC11); // по спадающему фронту. По низкому уровню не работает в спящем режиме, т.е. не считает!!!

	EIMSK |= (1 << INT1);

	if (light_level!=2) PORTC |= (1<<PORTC1); // подсветка вкл

	Init_LCD();


	scr=10;
	Draw_Screen();
	scr=0;
	_delay_ms(4000);
	nlcd_Clear();
	sei();


    while(1)
    {
		if (go_shutdown)
		{
			 go_shutdown=0;
			 // Гасим всё. То есть совсем всё.
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
			 PRR |= (1 << PRADC)|(1 << PRSPI)|(1 << PRTIM0); // отрубаем АЦП, СПИ, таймер писка
			 do_shutdown=1;
			 asm("sleep");
		}

		if (go_to_bed)	{go_to_bed=0; PinsOff();}

		if (out_of_bed)
		{
			out_of_bed=0;
			PinsOn();
			EIMSK &=~ (1 << INT0);
			PORTB &= ~(1<<PORTB0); //на всякий...
			PORTC |= (1<<PORTC0); // питание дисплея вкл
			if (light_level!=2) PORTC |= (1<<PORTC1); // подсветка вкл
			punkt=1;

			Init_LCD();

			redraw_LCD=1;
   		}

		if (!do_shutdown)
		{ // POWER OFF		

			if (recount)
			{
				recount=0;
				Count_Rad();
			}

			if ((awaken)&&(redraw_LCD)) 
			{
				redraw_LCD=0;
				Draw_Screen();
				if (!scr) Draw_Graph();
			}


			if ((batt)&&(awaken)) {batt=0; CheckBatt();}

			if (awaken) {CheckKey(); if (key) ReactKey();}
			if ((piip)&&(awaken)&&(beep_level)) {piip=0; Beep(14, 65);	}
	
		} // POWER OFF

		if ((!awaken)||(do_shutdown)) asm("sleep");
    }
}
