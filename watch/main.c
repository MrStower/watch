#include </avr_git/watch/minilib/minilib/library.h>

#define DEL 30
#define WAIT 10
#define MENU_LENGTH 5
uint8_t delay = DEL;
uint8_t state = 0x00;
static uint8_t statef = 0;
uint8_t bt_wait = WAIT;
uint8_t menu = 0;
uint8_t bt_wait_sel_u = WAIT;
uint8_t bt_wait_sel_d = WAIT;
uint8_t UART_cond = 0;
uint16_t addr = 0;
uint8_t shift = 8;
static uint8_t prev_menu = 0;
static uint8_t isr_count = 0;
uint8_t UART_arr[32];
uint8_t UART_pointer = 0;
uint8_t date_menu_line1[] = {41, 12, 6, 12, 6, 12};
uint8_t date_menu_line2[] = {46, 12, 6, 18};
uint8_t *lines[] = {date_menu_line1, date_menu_line2};
uint8_t param[] = {1, 1, 127 / 2 - 7 * 3, 127};
#define DATE_TIME_MENU_LINE1_LEN  (sizeof(date_menu_line1)) / 2
#define DATE_TIME_MENU_LINE2_LEN  (sizeof(date_menu_line2)) / 2
uint8_t date_time_menu_line_lengths[] = {DATE_TIME_MENU_LINE1_LEN, DATE_TIME_MENU_LINE2_LEN};
uint8_t alarm_menu_line1[] = {26, 12, 6, 12, 6, 12, 6, 18};
uint8_t alarm_menu_line2[] = {2, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12, 6, 12};
uint8_t *alarm_lines[] = {alarm_menu_line1, alarm_menu_line2, alarm_menu_line1, alarm_menu_line2};
#define ALARM_MENU_LINE1_3_LEN  (sizeof(alarm_menu_line1)) / 2
#define ALARM_MENU_LINE2_4_LEN  (sizeof(alarm_menu_line2)) / 2
uint8_t alarm_menu_line_lengths[] = {ALARM_MENU_LINE1_3_LEN, ALARM_MENU_LINE2_4_LEN, ALARM_MENU_LINE1_3_LEN, ALARM_MENU_LINE2_4_LEN};
uint8_t alarm_param[] = {0, 0, (127 - 5 * 6 - 2 * 6 - 5 * 6) / 2, 127};
uint8_t file;
uint8_t pages[40];
uint8_t pages_pointer;
uint8_t scroll;
#define HEADER_LEN 20
#define RECORD_START 3000
#define RECORD_LENGTH 1000
uint16_t record_cell_len[8] EEMEM;
uint8_t record_num EEMEM;
uint16_t file_offset;
static void alarm_stop(){
	status |= 0x30;
	TCCR1B = 0x00;
}
static void alarm_cont(){
	status &= ~0x30;
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
//returns how many symbols was written on lcd
static uint8_t read_page(){
	UART_SendChar(pages_pointer & 0x7F);
	lcd_res();
	file_offset = 0;
	for (uint8_t i = 0; i < (pages_pointer & 0x7F); i++)
		file_offset += pages[i];
	i2c_read_arr(EEP_ADDR, RECORD_START + file * RECORD_LENGTH - 5 + HEADER_LEN + file_offset, 174);
	ret_arr[175] = '\0';
	uint8_t temp_param[] = {0, 7, 0, 127};
	return word_out(temp_param, &ret_arr[5]);
}
inline static void show_menu(){
	lcd_res();
	switch(menu){	
		//date menu part
		case 21:
		case 22:
		case 23:
		case 2:
			comp_date();
			word_out(param, date_str);
			goto_page(3, 3);
			goto_x(46, 127);
			eep_str_write(ok_res, 6);
			cursor_v(0, lines, 2, DATE_TIME_MENU_LINE1_LEN + DATE_TIME_MENU_LINE2_LEN, date_time_menu_line_lengths);
		break;
		//time menu part
		case 31:
		case 32:
		case 33:
		case 3:
			comp_time();
			tempTime_str();
			word_out(param, time_str);
			goto_page(3, 3);
			goto_x(46, 127);
			eep_str_write(ok_res, 6);
			cursor_v(0, lines, 2, DATE_TIME_MENU_LINE1_LEN + DATE_TIME_MENU_LINE2_LEN, date_time_menu_line_lengths);
		break;
		//alarms part
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
			goto_x(27 + 30 + 6, 27 + 71);
			eep_str_write(on_off, 6);
			goto_page(4, 4);
			eep_str_write(on_off, 6);
			
			cursor_v(0, alarm_lines, 1, 2 * (ALARM_MENU_LINE1_3_LEN + ALARM_MENU_LINE2_4_LEN), alarm_menu_line_lengths);
			alarm_day_selection();
		break;
		//a reading menu
		case 6:;
			uint8_t i = 0;
			while (i < eeprom_read_byte(&record_num)){
				cli();
				i2c_read_arr(EEP_ADDR, RECORD_START + (i * RECORD_LENGTH) - 5, HEADER_LEN + 5);
				ret_arr[24] = 0;
				uint8_t temp_param[] = {i , i, 9, 127};
				word_out(temp_param, &ret_arr[5]);
				sei();
				i++;
			}
			cursor_horiz = 0;
			cursor_h(NOSTEP);
		break;
		case 60:;
			pages_pointer = 0;
			uint8_t readed = read_page();
			pages_pointer++;
			pages[pages_pointer] = readed;
		break;
	}
}
inline static void up_short(){
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
		case 6:
			cursor_h(UP);
		break;
		case 60:
			if (scroll){
				if (pages_pointer > 1)
					pages_pointer -= 2;
				else
					if (pages_pointer)
						pages_pointer--;
			} else
				if (pages_pointer)
					pages_pointer--;
			read_page();
			scroll = 0;
		break;
	}
	useful_out();
}
inline static void up_button(){
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
inline static void dn_short(){
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
				time_temp[0] = 23;
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
		case 6:
			cursor_h(DOWN);
		break;
		case 60:;
			file_offset += 2;
			uint16_t tempsize = eeprom_read_word(&(record_cell_len[file]));
			if (scroll == 0){
				if (file_offset < tempsize)
				pages_pointer++;
			}
			uint8_t readed = read_page();
			pages[pages_pointer] = readed;
			//UART_SendChar(pages_pointer);
			if (file_offset < tempsize)
				pages_pointer++;
			//UART_SendChar(pages_pointer);
			scroll = 1;
			file_offset -= 2;
		break;
	}
	useful_out();
}
inline static void dn_button(){
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
inline static void apply_changes(){
	switch (menu){
		case 24:
			ds3231_write_date();
			ds3231_read_date();
			menu = 2;
		break;
		case 25:
			comp_date();
			tempDate_str();
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
			tempTime_str();
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
inline static void ok_short(){
	alarm_stop();
	switch(menu){
		case 0:
			if(!delay)
				lcd_res();
			delay = DEL;
			display_time_date();
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
			if (cursor_v_pos < 4 || (cursor_v_pos > 10 && cursor_v_pos < 15))
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
		case 6:
			file = cursor_horiz;
			menu = 60;
			lcd_res();
			pages_pointer = 0;
			pages[0] = read_page();
			scroll = 1;
			pages_pointer = 1;
		break;
	}
	prev_menu = menu;
}
inline static void ok_long(){
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
		case 5:
		case 6:
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
		case 60:
			menu = 6;
			shiftout(DATA, 0x00); // doesn't work without it - why?
			cursor_v_pos = 0;
			lcd_res();
			show_menu();
			cursor_h(0);
		break;
	}
}
inline static void ok_button(){
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
			if (!(status & 0x30)){
				TCCR1B = 0x03;
			}
	}
}
/*So I discovered that I need something to do with more less freq that isr does*/
uint16_t* record_length_pointer; 
ISR(TIMER0_OVF_vect){
	//UART_SendStr((u_char*)"What\0");
	//sleep_disable(); how it's even work?
	TCCR0 = 0x00;
	sei();
	//PORTB^=0x01;
	if (UART_FLAG){
		if (record_length_pointer == NULL)
			record_length_pointer = &(record_cell_len[(addr - RECORD_START) / RECORD_LENGTH]);
			i2c_send_arr(EEP_ADDR, addr, UART_arr, UART_pointer);
			eeprom_write_word(record_length_pointer, eeprom_read_word(record_length_pointer) + UART_pointer);
			UART_SendStr((u_char *)"OK\0");
			if (UART_FLAG_FULL_FILE_SET){
				record_length_pointer = NULL;
				eeprom_write_byte(&record_num, eeprom_read_byte(&record_num) + 1);
			}
		UART_FLAG_RESET;
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
		PORTB ^= 0x02;
	}
	if (!(isr_count & 3) && !(status & 0x30)){
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
	DDRD = 0xE0;
	ok_button();
	up_button();
	dn_button();
	if (delay > 0 && !menu)
		delay--;
	else {
		if (!menu){
			shiftout(COM, 0xAE);
			DDRD = 0x00;
			}
	}
	TCCR0 = 0x04;
}

ISR(USART_RXC_vect){
	sleep_disable();
	uint8_t uartBuf = UDR;
	//UART_SendChar(uartBuf);
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
				if (uartBuf == 0x04){
					UART_cond = 0;
					UART_FLAG_FULL_FILE_SET;
					shift = 8;
				} else {
					UART_arr[UART_pointer] = uartBuf;
					UART_pointer++;
				}
			}
			break;
		case 4:
			eeprom_write_byte(&record_num, eeprom_read_byte(&record_num) - 1);
			UART_cond = 0;
			break;
	}
	switch (uartBuf){
		case 0x04: UART_cond = 4; break;
		case 0x02: UART_cond = 2; break;
		case 0x01: UART_cond = 1; break;
		//default: UART_cond = 0;
	}
}
ISR(TIMER1_COMPA_vect){
	cli();
	TCNT1L = 0;
	DDRB |= 0x01;
	PORTB ^= 0x01;
	DDRB ^= 0x01;
	sei();
}
int main(void){
	DDRD = 0xE0;
	PORTD = 0x00;
	DDRB = 0x02;
	ACSR |= (1 << ACD);
	i2c_init();
	ds3231_init();
	UART_Init();
	lcd_res();
	i2c_send_byte(DS_ADDR, 0x0E, 0x04); //disables BBSQW
	i2c_send_byte(DS_ADDR, 0x0F, 0x00); //disables 32kHz output
	ds3231_read_time();
	ds3231_read_date();
	display_time_date();
	draw_alarms();
	TIMSK = 0x11;
	TCCR0 = 0x04;
	TCNT0 = 0x00;
	TIFR = 0x10;
	OCR1AL = 0x0B;
	eeprom_write_byte(&record_num, 2);
	//record_num++;
	set_sleep_mode(SLEEP_MODE_IDLE);
	//ds3231_write_time(22, 8, 40);
	//ds3231_write_date(1, 8, 17);
	//u_char rec[] = "Record1 is the best Somebody once told me the world is gonna roll me, I ain't the sharpest tool in the shed. She was looking kind of dumb with her finger and her thumb in a shape of an \"L\" on her forehead. Well, the years start coming and they don't start coming, up to the rules and I hit the ground running. Doesn't make sense not to live for fun, your brain gets smart but your head gets dumb. So much to see, so much to do, so what's wrong not taking the back streets? You'll never know if you don't go, you'll never shine if you don't glow.\0";
	//eeprom_write_word(&record_cell_len[0], sizeof(rec));
	//i2c_send_arr(EEP_ADDR, RECORD_START, rec, sizeof(rec));
	while (1) {
		sleep_enable();
		sleep_cpu();
	}
}