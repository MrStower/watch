#include </avr_git/watch/minilib/minilib/library.c>

#define DEL 30
#define WAIT 10
#define MENU_LENGTH 5
uint8_t delay=DEL;
uint8_t state=0x00;
uint8_t bt_wait=WAIT;
uint8_t menu = 0;
uint8_t bt_wait_sel_u=WAIT;
uint8_t bt_wait_sel_d=WAIT;
uint8_t minutes_old=0;
uint8_t time[3];
uint8_t date[8];

void up_long(){
	
}
void up_short(){
	
}
void up_button(){
	if (!(PINB & 4)){
		state |= 0x10;
		if (!(state & 0x20)){
			bt_wait_sel_u = WAIT;
			state |= 0x20;
			} else 
				if(bt_wait_sel_u > 0)
					bt_wait_sel_u--;
		} else {
			if(state & 0x10){
				state &= 0xEF;
				if(bt_wait_sel_d > WAIT / 2)
					up_long();
				else
					up_short();
			}
		}
}
void dn_long(){
	
}
void dn_short(){
	
}
void dn_button(){
		if (!(PINB & 8)){
			state |= 0x40;
			if (!(state & 0x80)){
				bt_wait_sel_d = WAIT;
				state |= 0x80;
				} else 
					if(bt_wait_sel_d > 0)
						bt_wait_sel_d--;
			} else {
			if (state & 0x40){
				state &= 0xBF;
				if(bt_wait_sel_d > WAIT / 2)
					dn_long();
				else 
					dn_short();
			}
		}
}
void ok_short(){
	switch(menu){
		case 0:
		if(!delay)
			lcd_res();
		delay = DEL;
		ds3231_read_time(time);
		display_time(time);
		break;	
	}
}
void ok_long(){
	switch(menu){
		case 0:
			menu = 1;
			break;
		case 1:
			menu = 0;
			break;
	}
}
void ok_button(){
	if (!(PINB & 2)){
		state |= 0x02;
		if (!(state & 0x01)){
			bt_wait = WAIT;
			state |= 0x01;
			} else 
				if (bt_wait > 0) bt_wait--;
	} else {
		if (state&0x02){
			state&=0xFC;
			if (bt_wait > WAIT / 2)
				ok_short();
			else
				ok_long();			
		}
	}
}
ISR(TIMER0_OVF_vect){
	sleep_disable();
	DDRD = 0xE0;
	ok_button();
	up_button();
	dn_button();
	if (delay > 0)
		delay--;
	else {
		state &= 0xFB;
		if (!(state & 0x08)) shiftout(1, 0xAE);
		DDRD = 0x00;
	}
}
int main(void)
{
	DDRB = 0x01;
	DDRD = 0xE0;
	ACSR |= (1 << ACD);
	i2c_init();
	ds3231_init();
	lcd_res();
	ds3231_write_reg(0x0E, 0x04); //disables BBSQW
	ds3231_write_reg(0x0F, 0x00); //disables 32kHz output
	ds3231_read_time(time);
	display_time(time);
	TIMSK = 0x01;
	TCCR0 = 0x04;
	TCNT0 = 0x00;
	sei();
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	//ds3231_write_time(23, 38, 40);
	while (1) {
		sleep_enable();
		sleep_cpu();
	}
}