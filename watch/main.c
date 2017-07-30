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
uint8_t UART_cond = 0;
uint16_t addr = 0;
uint8_t shift = 8;
uint8_t UART_arr[32];
uint8_t UART_pointer = 0;
uint8_t date_menu_line1[] = {41, 12, 6, 12, 6, 12};
uint8_t date_menu_line2[] = {46, 12, 6, 18};
uint8_t *lines[] = {date_menu_line1, date_menu_line2};
uint8_t param[] = {1, 1, 127 / 2 - 7 * 3, 127, 0};
#define DATE_TIME_MENU_LINE1_LEN  (sizeof(date_menu_line1)) / 2
#define DATE_TIME_MENU_LINE2_LEN  (sizeof(date_menu_line2)) / 2
uint8_t date_time_menu_line_lengths[] = {DATE_TIME_MENU_LINE1_LEN, DATE_TIME_MENU_LINE2_LEN};
uint8_t alarm_menu_line1[] = {26, 12, 6, 12, 6, 12, 6, 18};
uint8_t alarm_menu_line2[] = {2, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12};
uint8_t *alarm_lines[] = {alarm_menu_line1, alarm_menu_line2, alarm_menu_line1, alarm_menu_line2};
#define ALARM_MENU_LINE1_3_LEN  (sizeof(alarm_menu_line1)) / 2
#define ALARM_MENU_LINE2_4_LEN  (sizeof(alarm_menu_line2)) / 2
uint8_t alarm_menu_line_lengths[] = {ALARM_MENU_LINE1_3_LEN, ALARM_MENU_LINE2_4_LEN, ALARM_MENU_LINE1_3_LEN, ALARM_MENU_LINE2_4_LEN};
uint8_t alarm_param[] = {0, 0, (127 - 5 * 6 - 2 * 6 - 5 * 6) / 2, 127, 0};
static void alarm_stop(){
	status |= 0x18;
	TCCR1B = 0x00;
}
static void alarm_cont(){
	status &= ~0x18;
}
static void alarm_day_selection(){
	for (uint8_t i = 0; i < 7; i++){
		uint8_t shift_menu = 2 + i * 18;
		if (alarm1_temp[2] & (1 << i)){
			if (cursor_v_pos == i + 4)
				fill_column(shift_menu, 3, 3, 12, 0x0B);
			else
				fill_column(shift_menu, 3, 3, 12, 0x0A);
		} else {
			if (cursor_v_pos == i + 4)
				fill_column(shift_menu, 3, 3, 12, 0x01);
		}
		if (alarm2_temp[2] & (1 << i)){
			if (cursor_v_pos == i + 15)
				fill_column(shift_menu, 7, 7, 12, 0x0B);
			else
				fill_column(shift_menu, 7, 7, 12, 0x0A);
		} else {
			if (cursor_v_pos == i + 15)
				fill_column(shift_menu, 7, 7, 12, 0x01);
		}
	}
}
static void set_alarm_day(){
	if (cursor_v_pos > 3 && cursor_v_pos < 11)
		alarm1_temp[2] ^= 1 << (cursor_v_pos - 4);
	if (cursor_v_pos > 14 && cursor_v_pos < 22)
		alarm2_temp[2] ^= 1 << (cursor_v_pos - 15);
}
static void useful_out(){
	if (menu / 10 == 2){
		tempDate_str();
		word_out(param, date_str);
		cursor_v(0, lines, 2, DATE_TIME_MENU_LINE1_LEN + DATE_TIME_MENU_LINE2_LEN, date_time_menu_line_lengths);
	}
	if (menu / 10 == 3){
		tempTime_str();
		word_out(param, time_str);
		cursor_v(0, lines, 2, DATE_TIME_MENU_LINE1_LEN + DATE_TIME_MENU_LINE2_LEN, date_time_menu_line_lengths);
	}
	if (menu > 99){
		alarmTemp_str();
		word_out(alarm_param, alarm_str1);
		alarm_param[0] += 4;
		alarm_param[1] += 4;
		word_out(alarm_param, alarm_str2);
		alarm_param[0] -= 4;
		alarm_param[1] -= 4;
		cursor_v(0, alarm_lines, 1, 2 * (ALARM_MENU_LINE1_3_LEN + ALARM_MENU_LINE2_4_LEN), alarm_menu_line_lengths);
	}
}
static void show_menu(){
	lcd_res();
	switch(menu){
		case 21:
		case 22:
		case 23:
		case 2:
			comp_date();
			word_out(param, date_str);
			goto_page(3, 3);
			goto_x(46, 127);
			eep_str_write(ok, 2);
			goto_page(3, 3);
			goto_x(46 + 18, 127);
			eep_str_write(res, 3);
			cursor_v(0, lines, 2, DATE_TIME_MENU_LINE1_LEN + DATE_TIME_MENU_LINE2_LEN, date_time_menu_line_lengths);
		break;
		case 31:
		case 32:
		case 33:
		case 3:
			comp_time();
			update_time_str();
			word_out(param, time_str);
			goto_page(3, 3);
			goto_x(46, 127);
			eep_str_write(ok, 2);
			goto_page(3, 3);
			goto_x(46 + 18, 127);
			eep_str_write(res, 3);
			cursor_v(0, lines, 2, DATE_TIME_MENU_LINE1_LEN + DATE_TIME_MENU_LINE2_LEN, date_time_menu_line_lengths);
		break;
		case 100:
		case 101:
		case 102:
		case 103:
		case 111:
		case 112:
		case 113:
		case 114:
		case 4:
			upd_alarm_str(3);
			word_out(alarm_param, alarm_str1);
			
			alarm_param[0] = 4;
			alarm_param[1] = 4;
			
			word_out(alarm_param, alarm_str2);
			
			alarm_param[0] = 0;
			alarm_param[1] = 0;
			
			goto_page(6, 6);
			goto_x((127 - 120) / 2, 127);
			eep_str_write(dow, 20);
			goto_page(2, 2);
			goto_x((127 - 120) / 2, 127);
			eep_str_write(dow, 20);
			
			goto_page(0,0);
			goto_x(27 + 30 + 6, 127);
			eep_str_write(on_off, 6);
			goto_page(4, 4);
			goto_x(27 + 30 + 6, 127);
			eep_str_write(on_off, 6);
			
			cursor_v(0, alarm_lines, 1, 2 * (ALARM_MENU_LINE1_3_LEN + ALARM_MENU_LINE2_4_LEN), alarm_menu_line_lengths);
			alarm_day_selection();
		break;
		case 6:;
	/*	char str[] = "это лайт чек на пере нос\0";
		uint8_t n_p[] = {0,7,0,127,1};
		word_out(n_p, (u_char*) str);*/
		break;
	}
}
static void up_long(){
	
}
static void up_short(){
	switch(menu){
		case 1:
			cursor_h(1);
		break;
		case 2:
		case 3:
			cursor_v(1, lines, 2, DATE_TIME_MENU_LINE1_LEN + DATE_TIME_MENU_LINE2_LEN, date_time_menu_line_lengths);
		break;
		case 21:
			date_temp[1]++;
			check_day_correct();
		break;
		case 22:
			if (date_temp[2] < 12)
				date_temp[2]++;
			else
				date_temp[2] = 1;
		break;
		case 23:
			if (date_temp[3] < 99)
				date_temp[3]++;
			else
				date_temp[3] = 0;
		break;
		case 31:
			if (time_temp[0] < 23)
				time_temp[0]++;
			else
				time_temp[0] = 0;
		break;
		case 32:
			if (time_temp[1] < 59)
				time_temp[1]++;
			else
				time_temp[1] = 0;
		break;
		case 33:
			if (time_temp[2] < 59)
				time_temp[2]++;
			else
				time_temp[2] = 0;
		break;
		case 4:
			cursor_v(1, alarm_lines, 1, 2 * (ALARM_MENU_LINE1_3_LEN + ALARM_MENU_LINE2_4_LEN), alarm_menu_line_lengths);
			alarm_day_selection();
		break;
		case 100:
			if (alarm1_temp[0] < 23)
				alarm1_temp[0]++;
			else
				alarm1_temp[0] = 0;
		break;
		case 101:
			if (alarm1_temp[1] < 59)
				alarm1_temp[1]++;
			else
				alarm1_temp[1] = 0;	
		break;
		case 111:
			if (alarm2_temp[0] < 23)
				alarm2_temp[0]++;
			else
				alarm2_temp[0] = 0;
		break;
		case 112:
			if (alarm2_temp[1] < 59)
				alarm2_temp[1]++;
			else
				alarm2_temp[1] = 0;
		break;
	}
	useful_out();
}
static uint8_t statef = 0;
static void up_button(){
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
static void dn_long(){

}
static void dn_short(){
	switch(menu){
		case 1:
			cursor_h(2);
		break;
		case 2:
		case 3:
			cursor_v(2, lines, 2, DATE_TIME_MENU_LINE1_LEN + DATE_TIME_MENU_LINE2_LEN, date_time_menu_line_lengths);
		break;
		case 4:
			cursor_v(2, alarm_lines, 1, 2 * (ALARM_MENU_LINE1_3_LEN + ALARM_MENU_LINE2_4_LEN), alarm_menu_line_lengths);
			alarm_day_selection();
		break;
		case 21:
			date_temp[1]--;
			check_day_correct();
		break;
		case 22:
			if (date_temp[2] > 1)
				date_temp[2]--;
			else
				date_temp[2] = 12;
		break;
		case 23:
			if (date_temp[3] > 0)
				date_temp[3]--;
			else
				date_temp[3] = 99;
		break;
		case 31:
			if (time_temp[0] > 0)
				time_temp[0]--;
			else
				time_temp[2] = 23;
		break;
		case 32:
			if (time_temp[1] > 0)
				time_temp[1]--;
			else
				time_temp[1] = 59;
		break;
		case 33:
			if (time_temp[2] > 0)
				time_temp[2]--;
			else
				time_temp[2] = 59;
		break;
		case 100:
			if (alarm1_temp[0] > 1)
				alarm1_temp[0]--;
			else
				alarm1_temp[0] = 23;
		break;
		case 101:
			if (alarm1_temp[1] > 1)
				alarm1_temp[1]--;
			else
				alarm1_temp[1] = 59;
		break;
		case 111:
			if (alarm2_temp[0] > 1)
				alarm2_temp[0]--;
			else
				alarm2_temp[0] = 23;
		break;
		case 112:
			if (alarm2_temp[1] > 1)
				alarm2_temp[1]--;
			else
				alarm2_temp[1] = 59;
		break;
	}
	useful_out();
}
static void dn_button(){
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
static void apply_changes(){
	switch (menu){
		case 24:
			ds3231_write_date(date_temp[1], date_temp[2], date_temp[3]);
			ds3231_read_date();
			menu = 2;
		break;
		case 25:
			comp_date();
			update_date_str();
			shiftout(DATA, 0x00); // why?
			menu = 2;
			show_menu();
		break;
		case 34:
			ds3231_write_time(time_temp[0], time_temp[1], time_temp[2]);
			ds3231_read_time();
			menu = 3;
		break;
		case 35:
			comp_time();
			update_time_str();
			shiftout(DATA, 0x00); // why?
			menu = 3;
			show_menu();
		break;
		case 102:
			alarm_set_reset(1, 1);
			menu = 4;
		break;
		case 103:
			alarm_set_reset(1, 0);
			upd_alarm_str(1);
			menu = 4;
			shiftout(DATA, 0x00);
			show_menu();
		break;
		case 113:
			alarm_set_reset(2, 1);
			menu = 4;
		break;
		case 114:
			alarm_set_reset(2, 0);
			upd_alarm_str(2);
			menu = 4;
			shiftout(DATA, 0x00);
			show_menu();
		break;
	}
}
static uint8_t prev_menu = 0;
static void ok_short(){
	alarm_stop();
	switch(menu){
		case 0:
			if(!delay)
				lcd_res();
			delay = DEL;
			display_time();
			display_date();
			draw_alarms();
		break;
		case 1:
			menu = cursor_horiz + 2;
			show_menu();
			cursor_horiz = 0;
			cursor_v_pos = 0;
		break;
		case 2:
			if (prev_menu == 1)
				comp_date();
			menu = 21 + cursor_v_pos;
			apply_changes();
		break;
		case 3:
			if (prev_menu == 1)
				comp_time();
			menu = 31 + cursor_v_pos;
			apply_changes();
		break;
		case 21:
		case 22:
		case 23:
			menu = 2;
		break;
		case 31:
		case 32:
		case 33:
			menu = 3;
		break;
		case 4:
			if (prev_menu == 1)
				upd_alarm_str(3);
			if (cursor_v_pos < 4 || (cursor_v_pos > 11 && cursor_v_pos < 15))
				menu = 100 + cursor_v_pos;
			else
				set_alarm_day();
			alarm_day_selection();
			apply_changes();
		break;
		case 100:
		case 101:
		case 111:
		case 112:
			menu = 4;
		break;
	}
	prev_menu = menu;
}
static void ok_long(){
	switch(menu){
		case 0:
			menu = 1;
			shiftout(DATA, 0x00); // doesn't work without it - why?
			lcd_res();
			show_menus();
			cursor_h(0);
		break;
		case 2:	
			ds3231_read_date();
			menu = 1;
			cursor_v_pos = 0;
			shiftout(DATA, 0x00); // doesn't work without it - why?
			lcd_res();
			show_menus();
			cursor_h(0);
		break;
		case 21:
		case 22:
		case 23:
			menu = 2;
		break;
		case 31:
		case 32:
		case 33:
			menu = 3;
		break;
		case 3:
			ds3231_read_time();
			menu = 1;
			cursor_v_pos = 0;
			shiftout(DATA, 0x00); // doesn't work without it - why?
			lcd_res();
			show_menus();
			cursor_h(0);
		break;
		case 4:
			menu = 1;
			shiftout(DATA, 0x00); // doesn't work without it - why?
			cursor_v_pos = 0;
			lcd_res();
			show_menus();
			cursor_h(0);
		break;
		case 1:
			menu = 0;
			lcd_res();
		break;
		case 100:
		case 101:
		case 111:
		case 112:
			menu = 4;
		break;
	}
}
static void ok_button(){
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
static void alarm_start(){
//	UART_SendChar((eeprom_read_byte(alarm1 + 2) | eeprom_read_byte(alarm1 + 2)) & 1 << date[0]);
	//if ((eeprom_read_byte(alarm1 + 2) | eeprom_read_byte(alarm2 + 2)) & 0x80 && ((eeprom_read_byte(alarm1 + 2) | eeprom_read_byte(alarm1 + 2)) & 1 << date[0])) {
	if ((eeprom_read_byte(alarm1 + 2) & 0x80 && (eeprom_read_byte(alarm1 + 2) & 1 << date[0])) || (eeprom_read_byte(alarm2 + 2) & 0x80 && (eeprom_read_byte(alarm2 + 2) & 1 << date[0]))) {
		//UART_SendChar(eeprom_read_byte(alarm1 + 2) & 1 << date[0]);
		if ((eeprom_read_byte(alarm1) == time[0] && eeprom_read_byte(alarm1 + 1) == time[1]) || (eeprom_read_byte(alarm2) == time[0] && eeprom_read_byte(alarm2 + 1) == time[1]))
			if (!(status & 0x18)){
				TCCR1B = 0x03;
			}
	}
}
static uint8_t isr_count = 0;
/*So I discovered that I need something to do with more less freq that isr does*/
ISR(TIMER0_OVF_vect){
	//sleep_disable(); how it's even work?
	TCCR0 = 0x00;
	sei();
	//PORTB^=0x01;
	if (UART_FLAG){
		if(!i2c_send_arr(EEP_ADDR, addr, UART_arr, UART_pointer)){
			UART_SendChar('O');
			UART_SendChar('K');
		}
		UART_FLAG_RESET;
/*		UART_SendChar(addr >> 8);
		UART_SendChar(addr);
		for (uint8_t r = 0; r < UART_pointer; r++){
			UART_SendChar(i2c_read_byte(EEP_ADDR, addr + r));
		}*/
		UART_pointer = 0;
	}
	if (!(isr_count % 10)){
		uint8_t temp_min = time[1]; //I need it when I redraw time on main screen
		uint8_t temp_h = time[0];
		ds3231_read_time(time);
		if (temp_min != time[1]){
			alarm_stop();
			alarm_cont();
			status |= 0x01;
		}
		if (temp_h > time[0])
			ds3231_read_date(date);
		alarm_start();
	}
	if (!(isr_count % 4) && !(status & 0x18)){
		TCNT1 = 0;
		switch(OCR1AL){
			case 0x10:
				OCR1AL = 0x0B;
			break;
			case 0x0B:
				OCR1AL = 0x30;
			break;
			case 0x30:
				OCR1AL = 0x0B;
			break;
		}
	}
	isr_count++;
	if 	(isr_count == 0xFF)
		isr_count = 0;
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
				UART_FLAG_SET;
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
ISR(TIMER1_COMPA_vect){
	sei();
	TCNT1L = 0;
	DDRB |= 0x01;
	PORTB ^= 0x01;
	cli();
}
int main(void){
	DDRD = 0xE0;
	PORTD = 0x00;
	//DDRB = 0x01;
	ACSR |= (1 << ACD);
	i2c_init();
	ds3231_init();
	UART_Init();
	lcd_res();
	i2c_send_byte(DS_ADDR, 0x0E, 0x04); //disables BBSQW
	i2c_send_byte(DS_ADDR, 0x0F, 0x00); //disables 32kHz output
	ds3231_read_time();
	ds3231_read_date();
	display_time();
	display_date();
	draw_alarms();
	TIMSK = 0x11;
	TCCR0 = 0x04;
	TCNT0 = 0x00;
	//TCCR1B = 0x03;
	TIFR = 0x10;
	OCR1AL = 0x0B;
	sei();
	set_sleep_mode(SLEEP_MODE_IDLE);
	//ds3231_write_time(22, 8, 40);
	//ds3231_write_date(1, 8, 17);
	while (1) {
		sleep_enable();
		sleep_cpu();
	}
}