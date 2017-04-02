#include <D:\avr_git\watch\minilib\minilib/library.c>
#define DS3231_SECONDS				0x00	// Адрес регистра секунд
#define DS3231_MINUTES				0x01	// Адрес регистра минут
#define DS3231_HOURS				0x02	// Адрес регистра часов
#define DS3231_DAY					0x03	// Адрес регистра дня недели
#define DS3231_DATE					0x04	// Адрес регистра числа
#define DS3231_MONTH				0x05	// Адрес регистра месяца
#define DS3231_YEAR					0x06	// Адрес регистра года
#define DS3231_ALARM_1_SEC			0x07	// Адрес регистра будильника 1 секунд
#define DS3231_ALARM_1_MIN			0x08	// Адрес регистра будильника 1 минут
#define DS3231_ALARM_1_HOR			0x09	// Адрес регистра будильника 1 часов
#define DS3231_ALARM_1_DAY			0x0A	// Адрес регистра будильника 1 дня недели
#define DS3231_ALARM_1_DAT			0x0A	// Адрес регистра будильника 1 числа
#define DS3231_ALARM_2_MIN			0x0B	// Адрес регистра будильника 2 минут
#define DS3231_ALARM_2_HOR			0x0C	// Адрес регистра будильника 2 часов
#define DS3231_ALARM_2_DAY			0x0D	// Адрес регистра будильника 2 вдя недели
#define DS3231_ALARM_2_DAT			0x0D	// Адрес регистра будильника 2 числа
#define DS3231_CONTROL				0x0E	// Адрес регистра управления
#define DS3231_STATUS				0x0F	// Адрес регистра состояния
#define DS3231_SET_CLOCK			0x10	// Адрес регистра корректировки частоты генератора времяни (0x81 - 0x7F)
#define DS3231_T_MSB				0x11	// Адрес регистра последней измеренной температуры старшиий байт
#define DS3231_T_LSB				0x12	// Адрес регистра последней измеренной температуры младшиий байт

// Режим вывода часов

#define DS3231_HOURS_12				0x40
#define DS3231_24					0x3F
#define DS3231_AM					0x5F
#define DS3231_PM					0x60
#define DS3231_GET_24				0x02
#define DS3231_GET_PM				0x01
#define DS3231_GET_AM				0x00

// Разряды регистров

#define DS3231_A1M1					0x07
#define DS3231_A1M2					0x07
#define DS3231_A1M3					0x07
#define DS3231_A1M4					0x07
#define DS3231_A2M2					0x07
#define DS3231_A2M3					0x07
#define DS3231_A2M4					0x07
#define DS3231_DY_DT				0x06
#define DS3231_EOSC					0x07
#define DS3231_BBSQW				0x06
#define DS3231_CONV					0x05
#define DS3231_INTCN				0x02
#define DS3231_A2IE					0x01
#define DS3231_A1IE					0x00
#define DS3231_OSF					0x07
#define DS3231_EN32KHZ				0x03
#define DS3231_BSY					0x02
#define DS3231_A2F					0x01
#define DS3231_A1F					0x00
#define DS3231_SIGN					0x07

// Вспомогательные данные функций

#define DS3231_ERR					0
#define DS3231_OK					1
#define DS3231_ON					1
#define DS3231_OFF					0

// Будильники

#define DS3231_ALARM_1				0x00
#define DS3231_ALARM_2				0x01
#define DS3231_ALARM_OFF			0x00
#define DS3231_ALARM_1_ON			0x01
#define DS3231_ALARM_2_ON			0x02
#define DS3231_ALARM_ALL_ON			0x03

#define DEL 50
#define WAIT 10
#define MENU_LENGTH 5
uint8_t delay=DEL;
uint8_t state=0x00;
uint8_t bt_wait=WAIT;
uint8_t menu_selected=0;
uint8_t menu_now=1;
uint8_t bt_wait_sel_u=WAIT;
uint8_t bt_wait_sel_d=WAIT;
uint8_t minutes_old=0;
uint8_t time[3];
uint8_t date[8];

void big_time(uint8_t hh,uint8_t hl,uint8_t mh, uint8_t ml){
	uint8_t time_arr[]={hh, hl,10, mh, ml};
	uint8_t latest=5;
	for (uint8_t w=0;w<5;w++){
		shiftout(1, 0x22);
		shiftout(1, 0x02);
		shiftout(1, 0x06);
		shiftout(1, 0x21);
		shiftout(1, latest);
		uint16_t end_point=0;
		if (w==2){
				end_point=266;
				latest-=20;
			}
			else{
				end_point=(time_arr[w]+1)*26;
			}
		shiftout(1, latest+25);
		latest+=28;
		for(uint8_t e=0;e<5;e++){
			for(uint16_t y=time_arr[w]*26;y<end_point;y++){
				//shiftout(0, pgm_read_byte(&font36px_line0[4-e+y*5]));
			}
		}
	}
}

void time_out(){
	ds3231_read_time(time);
	if(minutes_old!=time[1]){
		big_time(time[0]/10,time[0]%10,time[1]/10,time[1]%10);
		ds3231_read_date(date);
		for (uint8_t h=0;h<8;h++){
			if(h%2==0){date[7-h]=date[(7-h)>>1]%10;}
			else{date[7-h]=date[(7-h)>>1]/10;}
		}
		shiftout(1, 0x22);
		shiftout(1, 0x00);
		shiftout(1, 0x07);
		shiftout(1, 0x21);
		shiftout(1, 0x02);
		shiftout(1, 0x79);
		for(uint8_t u=2;u<8;u++){
			//for(uint8_t r=date[u]*5;r<(date[u]+1)*5;r++){shiftout(0, pgm_read_byte(&font8px[r]));}
			shiftout(0, 0x00);
			if(u%2==1&&u!=7){
				//for(uint8_t r=0;r<5;r++){shiftout(0, pgm_read_byte(&font8px[54-r]));}
			}
		}
	}
	minutes_old=time[1];
}

void sel_menu(uint8_t pos){
	menu_selected=pos;
	switch(pos){
		case 1:
		
		break;
		case 2:
		
		break;
		case 3:
		
		break;
		case 4:
		
		break;
	}
}

//dir 0-dn 1-up
void show_cursor(uint8_t dir){
	shiftout(1, 0x22);
	shiftout(1, menu_now-1);
	shiftout(1, menu_now-1);
	shiftout(1, 0x21);
	shiftout(1, 0x00);
	shiftout(1, 0x04);
//for(uint8_t r=0;r<5;r++){shiftout(0, pgm_read_byte(&font_small[69*5+r]));}
	
			shiftout(1, 0x22);
				if(dir==0){
					if(menu_now!=1){
						shiftout(1, menu_now-2);
						shiftout(1, menu_now-2);
					}else{
						shiftout(1, MENU_LENGTH-1);
						shiftout(1, MENU_LENGTH-1);
					}
				}else{
					if(menu_now==MENU_LENGTH){
						shiftout(1, 0x00);
						shiftout(1, 0x00);
						}else{
						shiftout(1, menu_now);
						shiftout(1, menu_now);
					}
				}
				shiftout(1, 0x21);
				shiftout(1, 0x00);
				shiftout(1, 0x07);
				for(uint8_t r=0;r<8;r++){
					shiftout(0, 0x00);
					}
	
}

void up_b(){
	if(state&0x08&&menu_selected==0){
	if(!(PINB&4)){
		state|=0x10;
		if(!(state&0x20)){
			bt_wait_sel_u=WAIT; state|=0x20;
			}
		else{
			if(bt_wait_sel_u>0){
				bt_wait_sel_u--;
				}
			}
		}else{
		if(state&0x10){
			state&=0xEF;
			if(bt_wait_sel_u>WAIT/2){
				state&=0xDF;
				if(menu_now<MENU_LENGTH){
					menu_now++;
					}
				else{
					menu_now=1;
					}
					//shiftout(0, 0x42);
			}else{
				state&=0xDF;
				}
			show_cursor(0x00);
			}
		}
	}
}
void dn_b(){
	if(state&0x08&&menu_selected==0){
		if(!(PINB&8)){
			state|=0x40;
			if(!(state&0x80)){bt_wait_sel_d=WAIT; state|=0x80;}
			else{if(bt_wait_sel_d>0){bt_wait_sel_d--;}}
			}else{
			if(state&0x40){
				state&=0xBF;
				if(bt_wait_sel_d>WAIT/2){
					state&=0x7F;
					if(menu_now>1){menu_now--;}
						else{menu_now=MENU_LENGTH;}
						//shiftout(0, 0x81);
				}else{
					state&=0x7F;
					}
				show_cursor(0x01);
			}
		}
	}
}
void short_press(){
	if(!(state&0x08)){
		if(!(state&0x04)){
			state|=0x04;
			lcd_res();
			minutes_old=0xff;
		}
		time_out();
		delay=DEL;
		}else{
		//sel_menu(menu_now);
		}
	};
//FRENCH BAUGETTE 	uint8_t baugette[]={49,26,5,24,28, 0xff,5,25,5, 0xff,29,18,8,21, 0xff,12,31,3,10,8,21, 0xff,20,16, 0,13,22,19,7,17,10,8,21, 0xff,1,19,11,14,10,64,4, 0, 0xff,2,27,15,5,9, 0xff,23, 0,30,65};	
void long_press(){
	if(!(state&0x08)){
		state|=0x08;
		lcd_res();
		menu_now=1;
		shiftout(0, 0xff);
		shiftout(0, 0xff);
	}
	else{
		state&=0xf7;/*shiftout(0, 0xaa);*/ shiftout(1, 0xAE);
		}
	};
//state |dn 1-st press|dn pressed|up 1-st press|up pressed|    in menu mode|led is running|press state|1-st press
void button_pwr(){
	if(!(PINB&2)){
		state|=0x02;
		if(!(state&0x01)){bt_wait=WAIT; state|=0x01;}
			else{if(bt_wait>0){bt_wait--;}}
	}else{
		if(state&0x02){
					state&=0xFC;
			if(bt_wait>WAIT/2){short_press();}
				else{shiftout(0, 0xff); long_press();}			
		}
	}
}
ISR(TIMER0_OVF_vect){
	/*sleep_disable();
	button_pwr();
	up_b();
	dn_b();
	if(delay>0){delay--;}else{state&=0xFB;  if(!(state&0x08))shiftout(1, 0xAE);}*/
}
int main(void)
{
	DDRB=0xFF;
	DDRD=0xFF;
	PORTD=0x00;
	ACSR |= (1 << ACD);
	
	//ds3231_init();
	//ds3231_write_reg(0x0E, 0x00);

/*set_sleep_mode(SLEEP_MODE_IDLE);
	TIMSK=0x01;
	TCCR0=0x04;
	TCNT0=0x00;
	sei();*/
	i2c_init();
	lcd_res();
	//i2c_search();
	uint8_t arr[] = "whatyqwera";
	/*i2c_send_arr(EEP_ADDR, 0x0200, arr, sizeof(arr));
	for (uint8_t r = 0; r < sizeof(arr); r++){
		shiftout(DATA, arr[r]);
	}
	_delay_ms(6);*/
	/*for (uint16_t r = 0; r < 1024; r++){
		//i2c_read_arr(EEP_ADDR, 0x0100 * 5 + r, 1);
		shiftout(DATA, i2c_read_byte(EEP_ADDR, 0x0021 * 5 + r));
		if(!((r + 1) % 5)) shiftout(DATA, 0x00);
	}*/
	shiftout(DATA, 0xAE);
	display_time();
	shiftout(DATA, 0xAE);
	while (1) {
	}
}