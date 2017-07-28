/*Will be moved to minilib.h*/
/*50 kHz I2C*/
#define F_CPU 1000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define u_char unsigned char


#define PRESCALER           0 //F_CPU/(16+2(PRESCALER)*4^0)
#define TWI_START    0
#define TWI_RESTART	 1
#define TWI_STOP            2
#define TWI_TRANSMIT        3
#define TWI_RECEIVE_ACK     4
#define TWI_RECEIVE_NACK    5
#define EEP_ADDR 0x54 //0b01010100
#define DS_ADDR  0x68 //0b01101000

/*DS registers*/
#define DS3231_SECONDS		0x00	// Seconds
#define DS3231_MINUTES		0x01	// Minutes
#define DS3231_HOURS		0x02	// Hours
#define DS3231_DAY		0x03	// Day of week
#define DS3231_DATE		0x04	// Day
#define DS3231_MONTH		0x05	// Month
#define DS3231_YEAR		0x06	// Year
#define DS3231_CONTROL		0x0E	// Control reg
#define DS3231_STATUS		0x0F	// Status reg
#define DS3231_SET_CLOCK	0x10	// Frequency correction
#define DS3231_T_MSB		0x11	// Hbyte temperature
#define DS3231_T_LSB		0x12	// Lbyte temperature

/*Display styles*/
#define DS3231_HOURS_12		0x40
#define DS3231_24		0x3F
#define DS3231_AM		0x5F
#define DS3231_PM		0x60
#define DS3231_GET_24		0x02
#define DS3231_GET_PM		0x01
#define DS3231_GET_AM		0x00

/*Other registers*/
#define DS3231_DY_DT		0x06
#define DS3231_EOSC		0x07
#define DS3231_BBSQW		0x06
#define DS3231_CONV		0x05
#define DS3231_INTCN		0x02
#define DS3231_OSF		0x07
#define DS3231_EN32KHZ		0x03
#define DS3231_BSY		0x02
#define DS3231_A2F		0x01
#define DS3231_A1F		0x00
#define DS3231_SIGN		0x07

#define COM 0x01
#define DATA 0x00
#define UP 0x01
#define DOWN 0x02
#define NOSTEP 0x00

#define MENU_DESC_START 2400

uint8_t ret_arr[196];
uint8_t res_seq[] EEMEM = {0xAE, 0xD5, 0xF0, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x00, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF};
uint8_t menu_shift[] EEMEM = {0, 5, 6, 11, 7, 6};
uint8_t font36px_shift[] EEMEM = {0, 22, 34, 57, 79, 105, 126, 147, 170, 192, 213, 219};
u_char ok[] EEMEM = "OK\0";
u_char res[] EEMEM = "RES\0";
u_char on_off[] EEMEM = "ON OFF\0";
u_char dow[] EEMEM = "Ïí Âò Ñð ×ò Ïò Ñá Âñ\0";
uint8_t cursor_horiz = 0;
uint8_t time[3];
uint8_t date[4];
u_char date_str[] = "dd/mm/yy\0";
u_char time_str[] = "hh:mm:ss\0";
uint8_t date_temp[4];
uint8_t time_temp[3];
uint8_t alarm1[3] EEMEM = {12, 0, 4};
uint8_t alarm2[3] EEMEM = {12, 0, 4};	
uint8_t alarm1_temp[3] = {12, 0, 4};
uint8_t alarm2_temp[3] = {12, 0, 4};
uint8_t alarm_str1[] = "hh:mm\0";
uint8_t alarm_str2[] = "hh:mm\0";
uint8_t status = 0; //Status reg:
/*
0 bit -> when minutes changed. I'm using that to redraw digits on the main screen. Resets in draw_big_digit function
1 bit -> alarm 1 set
2 bit -> alarm 2 set
3 bit -> UART flag
*/
#define UART_FLAG status & 0x08
#define UART_FLAG_SET status |= 0x08
#define UART_FLAG_RESET status &= ~0x08
void UART_SendChar(u_char sym)
{
	UDR = sym;
	while(!(UCSRA & (1 << UDRE)));
}
void shiftout(uint8_t type, uint8_t data){ //type: 0 goes for data, 1 goes for command
	cli();
	if (!type)
	PORTD |= 0b01000000;
	else
	PORTD &= 0b10111111;
	
	PORTD |= 0b00100000;
	PORTD &= 0b11011111;
	for(uint8_t i = 0; i < 8; i++){
		if (data & 0b10000000){
			PORTD |= 0b01000000;
			} else {
			PORTD &= 0b10111111;
		}
		PORTD |= 0b00100000;
		PORTD &= 0b11011111;
		data <<= 1;
	}
	sei();
}
/*Init I2C*/
void i2c_init(){
	TWSR &= ~( _BV(TWPS0) | _BV(TWPS1)); //divider
	TWBR = PRESCALER; //setting I2C frequency 
}
/*Goes to page. 4-byte end address|4-byte start address*/
void goto_page(uint8_t page_st, uint8_t page_end){
	cli();
	shiftout(COM, 0x22);
	shiftout(COM, page_st);
	shiftout(COM, page_end);
	sei();
}
/*Goes to x-dim. 4-byte end address|4-byte start address*/
void goto_x(uint8_t shift_f, uint8_t shift_s){
	cli();
	shiftout(COM, 0x21);
	shiftout(COM, shift_f);
	shiftout(COM, shift_s);
	sei();
}
/*Select action*/
uint8_t i2c_action(uint8_t action){
	switch(action){
		case TWI_START:
		case TWI_RESTART:
		TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWINT);
		break;
		case TWI_STOP:
		TWCR = _BV(TWSTO) | _BV(TWEN) | _BV(TWINT);
		break;
		case TWI_TRANSMIT:
		TWCR = _BV(TWEN) | _BV(TWINT);
		break;
		case TWI_RECEIVE_ACK:
		TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA);
		break;
		case TWI_RECEIVE_NACK:
		TWCR = _BV(TWEN) | _BV(TWINT);
		break;
	}
	/*Waits until transmission stops*/
	if(action != TWI_STOP)
		while (!(TWCR & _BV(TWINT)));
	switch(TWSR & 0xF8){
		case 0x20:
		case 0x38:
		case 0x48:
			return 1;
	}
	return 0;
}
/*Sends byte of data to I2C device*/
void i2c_search(){
	shiftout(DATA, 0xFF);
	for (uint8_t i2c_addr = 0x54; i2c_addr < 128; i2c_addr++){
		i2c_action(TWI_START);
		TWDR = i2c_addr << 1; // write
		shiftout(DATA, i2c_action(TWI_TRANSMIT));
		i2c_action(TWI_STOP);
	}
}
uint8_t i2c_send_byte(uint8_t i2c_addr, uint16_t device_addr, uint8_t data){
	cli();
	uint8_t err = 0;
	err += i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	err += i2c_action(TWI_TRANSMIT);
	if (i2c_addr != DS_ADDR){
		TWDR = device_addr >> 8;
		err += i2c_action(TWI_TRANSMIT);
	}
	TWDR = device_addr;
	err += i2c_action(TWI_TRANSMIT);
	TWDR = data;
	err += i2c_action(TWI_TRANSMIT);
	err += i2c_action(TWI_STOP);
	return err;
	sei();
}
/*Sends block of data to I2C device*/
uint8_t i2c_send_arr(uint8_t i2c_addr, uint16_t device_addr, uint8_t *data, uint8_t len){
	if (i2c_addr == EEP_ADDR && device_addr < 2400) return 1;
	i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	i2c_action(TWI_TRANSMIT);
	if (i2c_addr != DS_ADDR){
		TWDR = device_addr >> 8;
		i2c_action(TWI_TRANSMIT);
	}
	TWDR = device_addr;
	i2c_action(TWI_TRANSMIT);
	for (uint8_t i = 0; i < len; i++){
		TWDR = *(data + i);
		i2c_action(TWI_TRANSMIT);
	}
	i2c_action(TWI_STOP);
	return 0;
}
/*Reads byte of data from I2C device*/
uint8_t i2c_read_byte(uint8_t i2c_addr, uint16_t device_addr){
	uint8_t data = 0;
	uint8_t err = 0;
	err += i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	err += i2c_action(TWI_TRANSMIT);
	if (i2c_addr != DS_ADDR){
		TWDR = device_addr >> 8;
		err += i2c_action(TWI_TRANSMIT);
	}
	TWDR = device_addr;
	err += i2c_action(TWI_TRANSMIT);
	err += i2c_action(TWI_RESTART);
	TWDR = i2c_addr << 1 | 0x01; // read
	err += i2c_action(TWI_RECEIVE_ACK);
	err += i2c_action(TWI_RECEIVE_NACK);
	data = TWDR;
	err += i2c_action(TWI_STOP);
	return data;
}
/*Reads block of data from I2C device*/
void i2c_read_arr(uint8_t i2c_addr, uint16_t device_addr, uint8_t len){
	i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	i2c_action(TWI_TRANSMIT);
	if (i2c_addr != DS_ADDR){
		TWDR = device_addr >> 8;
		i2c_action(TWI_TRANSMIT);
	}
	TWDR = device_addr;
	i2c_action(TWI_TRANSMIT);
	i2c_action(TWI_RESTART);
	TWDR = i2c_addr << 1 | 0x01; // read
	i2c_action(TWI_RECEIVE_ACK);
	for (uint8_t i = 0; i < len - 1; i++){
		i2c_action(TWI_RECEIVE_ACK);
		ret_arr[i] = TWDR;
	}
	i2c_action(TWI_RECEIVE_NACK);
	ret_arr[len - 1] = TWDR;
	i2c_action(TWI_STOP);
}
void ds3231_init(void){
	uint8_t temp = 0;
	temp = i2c_read_byte(DS_ADDR, (0x1000 | DS3231_CONTROL)); //magic trick!
	i2c_send_byte(DS_ADDR, (0x1000 | DS3231_CONTROL), temp & 0x7F);
}
/*Because of tricky data format in RTC*/
uint8_t ds3231_byte(uint8_t data){
	return ((data / 10 << 4) | data % 10);
}
void ds3231_read_time(){	
	i2c_read_arr(DS_ADDR, DS3231_SECONDS, 3);
	*(time + 2) = ((ret_arr[0] & 0x70) >> 4) * 10 + (ret_arr[0] & 0x0F);
	*(time + 1) = ((ret_arr[1] & 0x70) >> 4) * 10 + (ret_arr[1] & 0x0F);
	*time = ((ret_arr[2] & 0x30) >> 4) * 10 + (ret_arr[2] & 0x0F);
}
void ds3231_write_time(uint8_t hours, uint8_t minutes, uint8_t seconds){
	uint8_t temparr[] = {ds3231_byte(seconds), ds3231_byte(minutes), ds3231_byte(hours)};
	i2c_send_arr(DS_ADDR, DS3231_SECONDS, temparr, 3);
}
void ds3231_read_date(){
	i2c_read_arr(DS_ADDR, (0x1000 | DS3231_DAY), 4);
	date[0] = ret_arr[0];
	date[1] = (ret_arr[1] & 0x0F) + (ret_arr[1] >> 4) * 10;
	date[2] = ((ret_arr[2] & 0x10) >> 4) * 10 + (ret_arr[2] & 0x0F);
	date[3] = (ret_arr[3] >> 4) * 10 + (ret_arr[3] & 0x0F);
	for (uint8_t i = 0; i < 3; i++){
		date_str[i * 3] = date[1 + i] / 10 + '0';
		date_str[i * 3 + 1] = date[1 + i] % 10 + '0';
	}
	date_str[8] = '\0';
}
void ds3231_write_date(uint8_t date, uint8_t day, uint8_t month, uint8_t year){
	uint8_t temparr[] = {ds3231_byte(date), ds3231_byte(day), ds3231_byte(month), ds3231_byte(year)};
	i2c_send_arr(DS_ADDR, DS3231_DAY, temparr, 4);
}
/*Display reset*/
void lcd_res(){
	cli();
	PORTD |= 0b10000000;
	_delay_us(250);
	PORTD &= 0b01111111;
	_delay_ms(5);
	PORTD |= 0b10000000;
	for (uint8_t u = 0; u < sizeof(res_seq) - 1; u++){
			shiftout(COM, eeprom_read_byte(&res_seq[u]));
	}
	for(uint16_t u = 0; u < 1024; u++){
		shiftout(DATA, 0x00);
	}
	shiftout(COM, eeprom_read_byte(&res_seq[sizeof(res_seq) - 1]));
	sei();
}
void eep_str_write(uint8_t* inp, uint8_t len){
	for (uint8_t i = 0; i < len; i++){
		i2c_read_arr(EEP_ADDR, (uint16_t) eeprom_read_byte(inp + i) * 5, 5);
		for (uint8_t r = 0; r < 5; r++){
			shiftout(DATA, ret_arr[r]);
		}
		shiftout(DATA, 0x00);
	}
}
/*Puts a string to lcd with small amount of parameters*/
uint8_t strlen_q(u_char *inp){
	uint8_t len = 0;
	while(*(inp + len)){
		len++;
	}
	return len;
}
/*3 param: 
	page: byte end address|byte start address
	x-cord: byte start address|byte end address
	control: 0 for no word shift control, 1 for proper shift
	*/
uint8_t word_out(uint8_t *param, u_char *input){
	uint8_t symbols = 0;
	uint8_t width = 0;
	uint8_t len = strlen_q(input);
	uint8_t curr_page = *param;
	goto_page(*(param), *(param + 1));
	goto_x(*(param + 2), *(param + 3));
	for (uint8_t w = 0; input[w]; w++){
		uint8_t q;
		if (len * 6 > (*(param + 3) - *(param + 2)) * (*(param + 1) - *(param) + 1))
			break;
		if (input[w] == ' ')
			for (q = w + 1; q <= len + 1; q++){
					if (input[q] == ' ' || !input[q]){
						if ((uint16_t)(q - w) * 6 > *(param + 3) - *(param + 2) - width * 6){
							if (curr_page == 7){
								break;
							} else {
								width = 0;
								curr_page++;
								w++;
								goto_page(curr_page, curr_page);
								goto_x(*(param + 2), *(param + 3));
							}
						}
						break;
					}
				}
		else {
			if (6 > *(param + 3) - *(param + 2) - width * 6){
				if (curr_page == 7){
					break;
					} else {
					width = 0;
					curr_page++;
					goto_page(curr_page, curr_page);
					goto_x(*(param + 2), *(param + 3));
				}
			}
		}
		i2c_read_arr(EEP_ADDR, (uint16_t) *(input + w) * 5, 5);
		for (uint8_t r = 0; r < 5; r++){
			shiftout(DATA, ret_arr[r]);
		}
		symbols++;
		width++;
		shiftout(DATA, 0x00);
	}
	return symbols;
}
void fill_column(uint8_t x_coord, uint8_t page_st, uint8_t page_end, uint8_t width, uint8_t type) {
	goto_x(x_coord, x_coord + width);
	goto_page(page_st, page_end);
	for (uint8_t i = 0; i <= width * 6; i++) shiftout(DATA, type);
}
void draw_big_digit(uint8_t num, uint8_t page, uint8_t x_coord){
	uint8_t space = 0;
	if (num < 10){
		if (num == 1){
			if (status & 0x01) fill_column(x_coord, 1, 6, 6, 0);
			x_coord += 7;
			space = 6;
		} else {
			if (status & 0x01) fill_column(x_coord, 1, 6, 2, 0);
			x_coord += 3;
			space = 2;
		}
	}	
	uint16_t shift = eeprom_read_byte(&font36px_shift[num]);
	uint16_t digit_len = (eeprom_read_byte(&font36px_shift[num + 1]) - shift);
	shift *= 5;
	i2c_read_arr(EEP_ADDR, 0x0500 + shift, digit_len * 5);
	for (uint8_t p = 0; p < 5; p++){
		goto_page(page + p, page + p);
		goto_x(x_coord, 127);
		for (uint8_t pos = 1; pos <= digit_len; pos++){
			shiftout(DATA,  ret_arr[5 * pos - 1 - p]);
		}
	}
	if (num != 10 && status & 0x01) fill_column(x_coord + digit_len, 1, 6, 26 - space - digit_len, 0);
	
}
void comp_date(){
	for(uint8_t i = 0; i < 4; i++){
		date_temp[i] = date[i];
	}
}
void comp_time(){
	for(uint8_t i = 0; i < 3; i++){
		time_temp[i] = time[i];
	}
}
void upd_alarm_str(){
	for (uint8_t i = 0; i < 2; i++){
		alarm_str1[i * 3] = eeprom_read_byte(alarm1 + i) / 10 + '0';
		alarm_str1[i * 3 + 1] =  eeprom_read_byte(alarm1 + i) % 10 + '0';
		alarm_str2[i * 3] =  eeprom_read_byte(alarm2 + i) / 10 + '0';
		alarm_str2[i * 3 + 1] =  eeprom_read_byte(alarm2 + i) % 10 + '0';
		alarm1_temp[i] = eeprom_read_byte(alarm1 + i);
		alarm2_temp[i] = eeprom_read_byte(alarm2 + i);
	}
}
/*Displays time*/
void display_time(){
	draw_big_digit(time[0] / 10, 1, 6);
	draw_big_digit(time[0] % 10, 1, 34);
	draw_big_digit(10, 1, 65);
	draw_big_digit(time[1] / 10, 1, 70);
	draw_big_digit(time[1] % 10, 1, 98);
	status &= ~0x01;
}
void display_date(){
	uint8_t param[] = {7, 7, 127 - 8 * 6, 127, 0, 0};
	word_out(param, date_str);
}
/*Rules the cursor in menu mode. 0 for just reset, 1 for up, 2 for down.*/
void cursor_h(uint8_t upd){
	uint8_t rem_pos = cursor_horiz;
	switch(upd){
		case 2:
			if (cursor_horiz < sizeof(menu_shift) - 2)
				cursor_horiz++;
			else
				cursor_horiz = 0;
			break;
		case 1:
			if (cursor_horiz > 0)
				cursor_horiz--;
			else
				cursor_horiz = sizeof(menu_shift) - 2;
			break;
	}
	uint8_t param[] = {cursor_horiz, cursor_horiz, 0, 10, 1};
	u_char pointer[] = ">\0";
	word_out(param, pointer);	
	if(upd){
		param[0] = rem_pos;
		param[1] = rem_pos;
		u_char null[] = " \0";
		word_out(param, null);
	}
}
uint8_t cursor_v_pos = 0;
void check_day_correct(){
		if ((date_temp[2] % 2 && date_temp[2] < 8) || (!(date_temp[2] % 2) && date_temp[2] > 7)){
			if (date_temp[1] > 31) date_temp[1] = 1;
			if (!date_temp[1]) date_temp[1] = 31;
		} else {
			if (date_temp[2] == 2){
				if ((((date_temp[3] + 2000) % 100) && !((date_temp[3] + 2000) % 4)) || !((date_temp[3] + 2000) % 400)){
					if (date_temp[1] > 29) date_temp[1] = 1;
					if (!date_temp[1]) date_temp[1] = 29;
				} else {
					if (date_temp[1] > 28) date_temp[1] = 1;
					if (!date_temp[1]) date_temp[1] = 28;
				}
			} else {
				if (date_temp[1] > 30) date_temp[1] = 1;
				if (!date_temp[1]) date_temp[1] = 30;
			}
		}
}
void draw_cursor(uint8_t cursor_pos_t, uint8_t** lines, uint8_t* line_lengths, uint8_t page_st, uint8_t fill){
	uint8_t i = 0;
	while(cursor_pos_t >= *(line_lengths + i)){
		cursor_pos_t -= *(line_lengths + i);
		i++;
	}
	UART_SendChar('i');
	UART_SendChar(i / 10 + '0');
	UART_SendChar(i % 10 + '0');
	UART_SendChar('\n');
	UART_SendChar('\r');
	/*if (i >= 2) {
		cursor_pos_t += *(line_lengths + i);
		i--;
	}*/
	uint8_t temp_sum = 0;
	for (uint8_t j = 0; j <= cursor_pos_t * 2; j++){
		temp_sum += *(*(lines + i) + j);
	}
	UART_SendChar('t');
	UART_SendChar(temp_sum / 100 + '0');
	UART_SendChar(temp_sum % 100 / 10 + '0');
	UART_SendChar(temp_sum % 10 + '0');
	UART_SendChar('\n');
	UART_SendChar('\r');
	fill_column(temp_sum, page_st + i * 2, page_st + i * 2, *(*(lines + i) + cursor_pos_t * 2 + 1), fill);
}
/*Rules cursor in vertical mode*/
/*Lines:
arrays of elem_pos && margin
page_st - page start
interval - 1 line
*/
void cursor_v(uint8_t upd, uint8_t** lines, uint8_t page_st, uint8_t menu_len, uint8_t* line_lengths){
	if (upd == 1){
		if (cursor_v_pos < menu_len - 1){
			//fill cursor element with 0
			draw_cursor(cursor_v_pos, lines, line_lengths, page_st, 0);
			//fill current pos + 1 with 1
			draw_cursor(++cursor_v_pos, lines, line_lengths, page_st, 1);
		} else {
			//fill cursor pos with 0
			draw_cursor(cursor_v_pos, lines, line_lengths, page_st, 0);
			//fill 0 with 1
			draw_cursor(0, lines, line_lengths, page_st, 1);
			cursor_v_pos = 0;
		}
	} else {
		if (upd){
			if (cursor_v_pos > 0){
				//fill cursor element with 0
				draw_cursor(cursor_v_pos, lines, line_lengths, page_st, 0);
				//fill current pos - 1 with 1
				draw_cursor(--cursor_v_pos, lines, line_lengths, page_st, 1);
				} else {
				//fill cursor pos with 0
				draw_cursor(cursor_v_pos, lines, line_lengths, page_st, 0);
				//fill max-elem with 1
				draw_cursor(menu_len - 1, lines, line_lengths, page_st, 1);
				cursor_v_pos = menu_len - 1;
			}
		} else {
			draw_cursor(cursor_v_pos, lines, line_lengths, page_st, 1);
		}
	}
}
/*Menus in menu select mode*/
void show_menus(){
	uint8_t len = 0;
	for (uint8_t i = 0; i < sizeof(menu_shift); i++){
		len += eeprom_read_byte(&menu_shift[i]);
	}
	i2c_read_arr(EEP_ADDR, MENU_DESC_START - 5, len + 5); //first 5 byte in ret_arr are for letter. So bad cheat
	uint8_t written = 0;
	uint8_t str = 0;
	while(str < sizeof(menu_shift) - 1){
		uint8_t param[] = {str, str, (127 - eeprom_read_byte(&menu_shift[str + 1]) * 6) / 2 + 3, 127, 1, 0};
		written += word_out(param, &ret_arr[written + 5]) + 1;
		str++;
	}
}
/*UART init*/
void UART_Init(void)
{
	UBRRH = 0;
	UBRRL = 12; //4800 baud
	UCSRB = (1 << RXCIE) | (1 << RXEN) | (1 << TXEN); //interrupt at receiving, tx/rx enable
	UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); //8-bit word
}







