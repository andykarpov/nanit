#ifndef config_h
#define config_h

// конфигурация пинов
#define PIN_PUMP         8 // PB0
#define PIN_OLED_CS      9 // PB1
#define PIN_OLED_DC     10 // PB2

#define PIN_BTN1         2 // PD2
#define PIN_SENSOR       3 // PD3
#define PIN_BTN2         4 // PD4
#define PIN_HV           5 // PD5 

#define PIN_OLED_VCC    14 // PC0
#define PIN_OLED_RESET  15 // PC1
#define PIN_BUZZER1     16 // PC2
#define PIN_BUZZER2     17 // PC3

// тональность звуковых сигналов
#define BEEP_TONE_ALARM 22
#define BEEP_TONE_KEY1  20
#define BEEP_TONE_KEY2  18
#define BEEP_TONE_PWR   14

// тип датчика
#define SENSOR_SBM10     0
#define SENSOR_SBM21     1
#define SENSOR_SBM20	32
#define SENSOR_SI19	    33
#define SENSOR_SBM21_2	34 // 2 счетчика СБМ-21
#define	SENSOR_M_SEL	40 // Выбор через настройки SBM10/SBM21

#ifndef SENSOR_TYPE
#define SENSOR_TYPE SENSOR_SBM10
#endif

// тип батареи
#define BATTERY_LION     0
#define BATTERY_NORMAL   1

#ifndef BATTERY_TYPE
#define BATTERY_TYPE BATTERY_LION
#endif

// тип предупреждения
#define ALARM_DOZE       0
#define ALARM_FON        1

#ifndef ALARM_TYPE
#define ALARM_TYPE ALARM_FON
#endif

// адреса в eeprom для хранения настроек
#define EEPROM_ADDR_BEEP_LEVEL 0
#define EEPROM_ADDR_SLEEP_LEVEL 2
#define EEPROM_ADDR_ALARM_LEVEL 4
#define EEPROM_ADDR_ION 8
#define EEPROM_ADDR_IMPULSE 16
#define EEPROM_ADDR_COUNTER 32

#endif
