void UART_SendChar(u_char sym){
	UDR = sym;
	while(!(UCSRA & (1 << UDRE)));
}
void UART_SendStr(u_char* str){
	while(*str)
		UART_SendChar(*(str++));
}
void send_sp(){
	uint16_t * s = SP;
	UART_SendChar(((uint16_t)s) / 1000 + '0');
	UART_SendChar(((uint16_t)s) % 1000 / 100 + '0');
	UART_SendChar(((uint16_t)s) % 100 / 10 + '0');
	UART_SendChar(((uint16_t)s) % 10 + '0');
	UART_SendChar('\n');
	UART_SendChar('\r');
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
static void i2c_action(uint8_t action){
	switch(action){
		case TWI_START:
		case TWI_RESTART:
		TWCR = _BV(TWSTA) | _BV(TWEN) | _BV(TWINT);
		break;
		case TWI_STOP:
		TWCR = _BV(TWSTO) | _BV(TWEN) | _BV(TWINT);
		break;
		case TWI_RECEIVE_ACK:
		TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA);
		break;
		case TWI_TRANSMIT:
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
			status |= 0x40;
			//UART_SendStr((u_char*)"i2c_fault\n\0");
		break;
	}
}
/*Sends byte of data to I2C device*/
void i2c_send_byte(uint8_t i2c_addr, uint16_t device_addr, uint8_t data){
	cli();
	//uint8_t err = 0;
	i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	i2c_action(TWI_TRANSMIT);
	if (i2c_addr != DS_ADDR){
		TWDR = device_addr >> 8;
		i2c_action(TWI_TRANSMIT);
	}
	TWDR = device_addr;
	i2c_action(TWI_TRANSMIT);
	TWDR = data;
	i2c_action(TWI_TRANSMIT);
	i2c_action(TWI_STOP);
	//return err;
	sei();
}
/*Sends block of data to I2C device*/
void i2c_send_block(uint8_t i2c_addr, uint16_t device_addr, uint8_t *data, uint8_t len){
	cli();
	//if ((i2c_addr == EEP_ADDR) && (device_addr < 2400)){
		//there might be your exception
	//	} else {
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
//	}
	sei();
}
uint8_t i2c_send_arr(uint8_t i2c_addr, uint16_t device_addr, uint8_t *data, uint16_t len){
	if (device_addr % 32){
		uint8_t shift = 32 - (device_addr % 32);
		if (len < shift){
			i2c_send_block(i2c_addr, device_addr, data, len);
			return 0;
		} else {
			i2c_send_block(i2c_addr, device_addr, data, shift);
			len -= shift;
			device_addr += shift;
			data += shift;
			_delay_ms(6);
		}
	}
	for (uint8_t i = 0; i < len >> 5; i++){
		i2c_send_block(i2c_addr, device_addr, data, 32);
		device_addr += 32;
		data += 32;
		_delay_ms(6);
	}
	if (len % 32){
		i2c_send_block(i2c_addr, device_addr, data, len % 32);
		_delay_ms(6);
		return 0;
	}
	return 1;
}
/*Reads byte of data from I2C device*/
uint8_t i2c_read_byte(uint8_t i2c_addr, uint16_t device_addr){
	uint8_t data = 0;
	//uint8_t err = 0;
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
	i2c_action(TWI_RECEIVE_NACK);
	data = TWDR;
	i2c_action(TWI_STOP);
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
	uint8_t temp = i2c_read_byte(DS_ADDR, (0x1000 | DS3231_CONTROL)); //magic trick!
	i2c_send_byte(DS_ADDR, (0x1000 | DS3231_CONTROL), temp & 0x7F);
}
/*Because of tricky data format in RTC*/
uint8_t ds3231_byte(uint8_t data){
	return ((data / 10 << 4) | data % 10);
}
void comp_date(){
	for(uint8_t i = 0; i < 4; i++){
		date_temp[i] = date[i];
	}
}
void tempDate_str(){
	for (uint8_t i = 0; i < 3; i++){
		date_str[i * 3] = date_temp[1 + i] / 10 + '0';
		date_str[i * 3 + 1] = date_temp[1 + i] % 10 + '0';
	}
}
void tempTime_str(){
	for (uint8_t i = 0; i < 3; i++){
		time_str[i * 3] = time_temp[i] / 10 + '0';
		time_str[i * 3 + 1] = time_temp[i] % 10 + '0';
	}
}
void comp_time(){
	for(uint8_t i = 0; i < 3; i++){
		time_temp[i] = time[i];
	}
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
	comp_date();
	tempDate_str();
}
void ds3231_write_date(){
	uint8_t a = (14 - date_temp[2]) / 12;
	uint16_t y = 2000 + date_temp[3] - a;
	uint8_t m = date_temp[2] + 12 * a - 2;
	date_temp[0] = (date_temp[1] + y + y / 4 - y / 100 + y / 400 + 31 * m / 12) % 7; //day of the week? omg
	if (date_temp[0])
		date_temp[0]--;
	else
		date_temp[0] = 6;
	uint8_t temparr[] = {ds3231_byte(date_temp[0]), ds3231_byte(date_temp[1]), ds3231_byte(date_temp[2]), ds3231_byte(date_temp[3])};
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
	while(*(inp + len++));
	return len;
}
/*3 param: 
	page: byte end address|byte start address
	x-cord: byte start address|byte end address
	*/
#define PAGE_START *param
#define PAGE_END *(param + 1)
#define X_START *(param + 2)
#define X_END *(param + 3)
uint8_t word_out(uint8_t *param, u_char *input){
	uint8_t symbols = 0;
	uint8_t width = 0;
	uint8_t len = strlen_q(input);
	uint8_t curr_page = PAGE_START;
	goto_page(PAGE_START, PAGE_END);
	goto_x(X_START, X_END);
	for (uint8_t w = 0; input[w]; w++){
		uint8_t q;
		if (input[w] == ' '){
			for (q = w + 1; q <= len + 1; q++){
				if (input[q] == ' ' || !input[q]){
					if ((uint16_t)(q - w) * 6 > X_END - X_START - width * 6){
						symbols++;
						if (curr_page == 7){
							return symbols;
						} else {
							width = 0;
							curr_page++;
							w++;
							goto_page(curr_page, curr_page);
							goto_x(X_START, X_END);
						}
					}
					break;
				}
			}
		} else {
			if (6 > X_END - X_START - width * 6){
				if (curr_page == 7){
					break;
				} else {
					width = 0;
					curr_page++;
					goto_page(curr_page, curr_page);
					goto_x(X_START, X_END);
				}
			}
		}
		i2c_read_arr(EEP_ADDR, (uint16_t) *(input + w) * 5, 5);
		for (uint8_t r = 0; r < 5; r++){
			shiftout(DATA, ret_arr[r]);
		}
		//_delay_ms(100);
		symbols++;
		width++;
		shiftout(DATA, 0x00);
	}
	return symbols;
}
static void fill_column(uint8_t x_coord, uint8_t page_st, uint8_t page_end, uint8_t width, uint8_t type) {
	goto_x(x_coord, x_coord + width);
	goto_page(page_st, page_end);
	for (uint8_t i = 0; i <= width * 6; i++) shiftout(DATA, type);
}
static void al_dr(uint8_t *alrm){
	for (uint8_t i = 0; i < 30; i++){
			if (i < 15){
				shiftout(DATA, eeprom_read_byte(alrm + i * 2 + 1));
				} else {
				shiftout(DATA, eeprom_read_byte(alrm + i * 2 - 30));
			}
		}
}
void draw_alarms(){
	if (eeprom_read_byte(alarm1 + 2) & 0x80){
		goto_page(6, 7);
		goto_x(0, 14);
		al_dr(alarm1_img);
	}
	if (eeprom_read_byte(alarm2 + 2) & 0x80){
		goto_page(6, 7);
		goto_x(16, 30);
		al_dr(alarm2_img);
	}
}
static void draw_big_digit(uint8_t num, uint8_t x_coord){
	//uint8_t space = 0;
	if (num < 10){
		if (num == 1){
			//if (status & 0x01) fill_column(x_coord, 1, 6, 6, 0);
			x_coord += 7;
			//space = 6;
		} else {
			//if (status & 0x01) fill_column(x_coord, 1, 6, 2, 0);
			x_coord += 3;
			//space = 2;
		}
	}	
	uint16_t shift = eeprom_read_byte(&font36px_shift[num]);
	uint16_t digit_len = (eeprom_read_byte(&font36px_shift[num + 1]) - shift);
	shift *= 5;
	i2c_read_arr(EEP_ADDR, 0x0500 + shift, digit_len * 5);
	for (uint8_t p = 0; p < 5; p++){
		goto_page(/*page + */p, /*page + */p);
		goto_x(x_coord, 127);
		for (uint8_t pos = 1; pos <= digit_len; pos++){
			shiftout(DATA,  ret_arr[5 * pos - 1 - p]);
		}
	}
	//if (num != 10 && status & 0x01) fill_column(x_coord + digit_len, 1, 6, 26 - space - digit_len, 0);
	
}

void alarmTemp_str(){
	for (uint8_t i = 0; i < 2; i++){
		alarm_str1[i * 3] = alarm1_temp[i] / 10 + '0';
		alarm_str1[i * 3 + 1] =  alarm1_temp[i] % 10 + '0';
		alarm_str2[i * 3] =  alarm2_temp[i] / 10 + '0';
		alarm_str2[i * 3 + 1] =  alarm2_temp[i] % 10 + '0';
	}
}
void upd_alarm_str(uint8_t num){
	for (uint8_t i = 0; i < 2; i++){
		if (num & 1){
			//alarm_str1[i * 3] = eeprom_read_byte(alarm1 + i) / 10 + '0';
			//alarm_str1[i * 3 + 1] =  eeprom_read_byte(alarm1 + i) % 10 + '0';
			alarm1_temp[i] = eeprom_read_byte(alarm1 + i);
		}
		if (num & 2){
			//alarm_str2[i * 3] =  eeprom_read_byte(alarm2 + i) / 10 + '0';
			//alarm_str2[i * 3 + 1] =  eeprom_read_byte(alarm2 + i) % 10 + '0';
			alarm2_temp[i] = eeprom_read_byte(alarm2 + i);
		}
	}
	alarm1_temp[2] = eeprom_read_byte(alarm1 + 2);
	alarm2_temp[2] = eeprom_read_byte(alarm2 + 2);
	alarmTemp_str();
}
void alarm_set_reset(uint8_t num, uint8_t sres){
	if (num == 1){
		if (sres)
			alarm1_temp[2] |= 0x80;
		else
			alarm1_temp[2] &= ~0x80;
		eeprom_update_block(alarm1_temp, alarm1, 3);
	} else {
		if (sres)
			alarm2_temp[2] |= 0x80;
		else
			alarm2_temp[2] &= ~0x80;
		eeprom_update_block(alarm2_temp, alarm2, 3);
	}
}
/*Displays time*/
void display_time_date(){
	draw_big_digit(time[0] / 10, 6);
	draw_big_digit(time[0] % 10, 34);
	draw_big_digit(10, 63);
	draw_big_digit(time[1] / 10, 70);
	draw_big_digit(time[1] % 10, 98);
	uint8_t param[] = {7, 7, 127 - 8 * 6, 127};
	word_out(param, date_str);
	status &= ~0x01;
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
	word_out(param,(u_char*)">\0");	
	if(upd){
		param[0] = rem_pos;
		param[1] = rem_pos;
		word_out(param, (u_char*)" \0");
	}
}
void check_day_correct(){
	uint8_t max_day = 31;
	if (((date_temp[2] & 1) && date_temp[2] < 8) || (!(date_temp[2] & 1) && date_temp[2] > 8)){
		if (date_temp[2] == 2){
			max_day = 28;
			if (date_temp[2] % 4)
				max_day = 29;
		}
	} else
		max_day = 30;
		/*if (((date_temp[2] & 1) && date_temp[2] < 8) || (!(date_temp[2] % 2) && date_temp[2] > 7)){
			max_day = 31;
		} else {
			if (date_temp[2] == 2){
				//if ((((date_temp[3] + 2000) % 100) && !((date_temp[3] + 2000) % 4)) || !((date_temp[3] + 2000) % 400)){
				//Truly check for year
				//lite version - till year 2099
				if (!(date_temp[3] % 4))
					max_day = 29;
				else
					max_day = 28;
			} else {
				max_day = 30;
			}
		}*/
	if (date_temp[1] > max_day)
		date_temp[1] = 1;
	else
		if (!date_temp[1]) date_temp[1] = max_day;
}
static void draw_cursor(uint8_t cursor_pos_t, uint8_t** lines, uint8_t* line_lengths, uint8_t page_st, uint8_t fill){
	uint8_t i = 0;
	while(cursor_pos_t >= *(line_lengths + i)){
		cursor_pos_t -= *(line_lengths + i);
		i++;
	}
	uint8_t temp_sum = 0;
	for (uint8_t j = 0; j <= cursor_pos_t * 2; j++){
		temp_sum += *(*(lines + i) + j);
	}
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
		uint8_t param[] = {str, str, (66 - eeprom_read_byte(&menu_shift[str + 1]) * 3), 127, 1, 0};
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







