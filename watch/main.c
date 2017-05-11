#include </avr_git/watch/minilib/minilib/library.c>

#define DEL 30
#define WAIT 10
#define MENU_LENGTH 5
uint8_t delay = DEL;
uint8_t state = 0x00;
uint8_t bt_wait = WAIT;
uint8_t menu = 0;
uint8_t bt_wait_sel_u = WAIT;
uint8_t bt_wait_sel_d = WAIT;
uint8_t time[3];
uint8_t date[8];
uint8_t UART_flag = 0;
uint8_t UART_cond = 0;
uint8_t uartBuf = 0;
uint16_t addr = 0;
uint8_t shift = 8;
uint8_t UART_arr[32];
uint8_t UART_pointer = 0;

void up_long(){
	PORTB^=0x01;
}
void up_short(){
	switch(menu){
		case 1:
		cursor_h(1);
		break;
	}
}
uint8_t statef = 0;
void up_button(){
	if (!(PIND & 8)){
		statef |= 0x10;
		if (!(statef & 0x20)){
			bt_wait_sel_u = WAIT;
			statef |= 0x20;
			} else 
				if(bt_wait_sel_u > 0)
					bt_wait_sel_u--;
		} else {
			if(statef & 0x10){
				statef &= ~0x30;
				if(bt_wait_sel_d > WAIT / 2)
					up_short();
			}
		}
}
void dn_long(){
	PORTB^=0x01;
}
void dn_short(){
	switch(menu){
		case 1:
		cursor_h(2);
		break;
	}
}
void dn_button(){
		if (!(PIND & 4)){
			statef |= 0x40;
			if (!(statef & 0x80)){
				bt_wait_sel_d = WAIT;
				statef |= 0x80;
				} else 
					if(bt_wait_sel_d > 0)
						bt_wait_sel_d--;
		} else {
			if (statef & 0x40){
				statef &= ~0xD0;
				if(bt_wait_sel_d > WAIT / 2)
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
			display_time(time);
			break;
		case 1:
			shiftout(DATA, 0xae);
			break;
	}
}
void ok_long(){
	switch(menu){
		case 0:
			lcd_res();
			goto_page(0,7);
			goto_x(0,127);
			show_menus();
			cursor_h(1);
			menu = 1;
			break;
		case 1:
			menu = 0;
			lcd_res();
			break;
	}
}
void ok_button(){
	if (!(PIND & 16)){
		state |= 0x02;
		if (!(state & 0x01)){
			bt_wait = WAIT;
			state |= 0x01;
			} else 
				if (bt_wait > 0)
					bt_wait--;
	} else {
		if (state & 0x02){
			state &= ~0x03;
			if (bt_wait > WAIT / 2)
				ok_short();
			else
				ok_long();			
		}
	}
}

ISR(TIMER0_OVF_vect){
	sleep_disable();
	TCCR0 = 0x00;
	TCNT0++;
	sei();
	PORTB^=0x01;
	if (UART_flag){
		if(!i2c_send_arr(EEP_ADDR, addr, UART_arr, UART_pointer)){
			UART_SendChar('O');
			UART_SendChar('K');
		}
		UART_flag = 0;
		UART_SendChar(addr>>8);
		UART_SendChar(addr);
		for (uint8_t r = 0; r < UART_pointer; r++){
			UART_SendChar(i2c_read_byte(EEP_ADDR, addr + r));
		}
		UART_pointer = 0;
	}
	ds3231_read_time(time);
	DDRD = 0xE0;
	ok_button();
	up_button();
	dn_button();
	if (delay > 0 && !menu)
		delay--;
	else {
		if (!menu){
			shiftout(1, 0xAE);
			DDRD = 0x00;
			}
	}
	TCCR0 = 0x04;
}

ISR(USART_RXC_vect){
	sleep_disable();
	uint8_t uartBuf = UDR;
	switch (UART_cond){
		case 1:
			addr |= uartBuf << shift;
			shift -= 8;
			if (shift){
				UART_cond = 0;
			}
			break;
		case 2:
			if (uartBuf == 0x03){ // End of text transmission
				UART_cond = 0;
				UART_flag = 1;
				shift = 8;
			} else {
				UART_arr[UART_pointer] = uartBuf;
				UART_pointer++;
			}
			break;
	}
	switch (uartBuf){
		case 0x02: UART_cond = 2; break;
		case 0x01: UART_cond = 1; break;
		//default: UART_cond = 0;
	}
}
int main(void)
{
	DDRD = 0xE0;
	PORTD = 0x00;
	DDRB = 0x01;
	ACSR |= (1 << ACD);
	i2c_init();
	ds3231_init();
	UART_Init();
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