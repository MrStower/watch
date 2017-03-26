/*Will be moved to minilib.h*/
/*50 kHz I2C*/
#define F_CPU 1000000
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

#define PRESCALER           2 //F_CPU/(16+2(PRESCALER)*4^0)
#define TWI_START    0
#define TWI_RESTART	 1
#define TWI_STOP            2
#define TWI_TRANSMIT        3
#define TWI_RECEIVE_ACK     4
#define TWI_RECEIVE_NACK    5
#define EEP_ADDR 0x52 //0b01010010
#define DS_ADDR  0xD0 //0b11010000

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

uint8_t ret_arr[32];
uint8_t res_seq[] EEMEM = {0xAE, 0xD5, 0xF0, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x00, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF};

/*Init I2C*/
static void i2c_init(){
	TWSR &= ~( _BV(TWPS0) | _BV(TWPS1)); //divider
	TWBR = PRESCALER; //setting I2C frequency 
}
/*Select action*/
static uint8_t i2c_action(uint8_t action){
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
	return (TWSR & 0xF8);
}
/*Sends byte of data to I2C device*/
static int i2c_send_byte(uint8_t i2c_addr, uint16_t device_addr, uint8_t data){
	uint8_t err = 0;
	err += !!i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	err += !!i2c_action(TWI_TRANSMIT);
	TWDR = device_addr >> 8;
	err += !!i2c_action(TWI_TRANSMIT);
	if (!(device_addr & 0x1000)){
		TWDR = device_addr;
		err += !!i2c_action(TWI_TRANSMIT);
	}
	TWDR = data;
	err += !!i2c_action(TWI_TRANSMIT);
	err += !!i2c_action(TWI_STOP);
	return err;
}
/*Sends block of data to I2C device*/
static int i2c_send_arr(uint8_t i2c_addr, uint16_t device_addr, uint8_t *data, uint8_t len){
	uint8_t err = 0;
	err += !!i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	err += !!i2c_action(TWI_TRANSMIT);
	TWDR = device_addr >> 8;
	err += !!i2c_action(TWI_TRANSMIT);
	TWDR = device_addr;
	err += !!i2c_action(TWI_TRANSMIT);
	for (uint8_t i = 0; i < len; i++){
		TWDR = *(data + i);
		err += !!i2c_action(TWI_TRANSMIT);
	}
	err += !!i2c_action(TWI_STOP);
	return err;
}
/*Reads byte of data from I2C device*/
static int i2c_read_byte(uint8_t i2c_addr, uint16_t device_addr){
	uint8_t err = 0;
	uint8_t data = 0;
	err += !!i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	err += !!i2c_action(TWI_TRANSMIT);
	TWDR = (device_addr & 0x7FFF) >> 8;
	err += !!i2c_action(TWI_TRANSMIT);
	if (!(device_addr & 0x1000)){
		TWDR = device_addr;
		err += !!i2c_action(TWI_TRANSMIT);	
	}
	err += !!i2c_action(TWI_RESTART);
	TWDR = i2c_addr << 1 | 0x01; // read
	err += !!i2c_action(TWI_RECEIVE_NACK);
	data = TWDR;
	err += !!i2c_action(TWI_STOP);
	return data;
}
/*Reads block of data from I2C device*/
static uint8_t i2c_read_arr(uint8_t i2c_addr, uint16_t device_addr, uint8_t len){
	uint8_t err = 0;
	err += !!i2c_action(TWI_START);
	TWDR = i2c_addr << 1; // write
	err += !!i2c_action(TWI_TRANSMIT);
	TWDR = device_addr >> 8;
	err += !!i2c_action(TWI_TRANSMIT);
	TWDR = device_addr;
	err += !!i2c_action(TWI_TRANSMIT);
	err += !!i2c_action(TWI_RESTART);
	TWDR = i2c_addr << 1 | 0x01; // read
	err += !!i2c_action(TWI_RECEIVE_ACK);
	for (uint8_t i = 0; i < len - 1; ++i){
		err += !!i2c_action(TWI_RECEIVE_ACK);
		ret_arr[i] = TWDR;
	}
	err += !!i2c_action(TWI_RECEIVE_NACK);
	ret_arr[len] = TWDR;
	err += !!i2c_action(TWI_STOP);
	return err;
}
void ds3231_init(void){
	uint8_t temp = 0;
	temp = i2c_read_byte(DS_ADDR, (0x1000 | DS3231_CONTROL)); //magic trick!
	i2c_send_byte(DS_ADDR, (0x1000 | DS3231_CONTROL), temp & 0x7F);
}
void ds3231_write_reg(uint8_t reg, uint8_t data){
	i2c_send_byte(DS_ADDR, (0x1000 | reg), data);
}
static uint8_t ds3231_read_reg(uint8_t reg){
	uint8_t temp = 0;
	temp = i2c_read_byte(DS_ADDR, (0x1000 | reg));
	return temp;
}
/*Because of tricky data format in RTC*/
static uint8_t ds3231_byte(uint8_t data){
	uint8_t temp = 0;
	
	while(data > 9)
	{
		data -= 10;
		temp++;
	}
	
	return (data | (temp << 4));
}
static uint8_t ds3231_read_time(uint8_t *str){
	uint8_t temp[3];
	uint8_t i = 0;
	uint8_t hour = 0;
	
	i2c_read_arr(DS_ADDR, (0x1000 | DS3231_SECONDS), 3);
	temp[2] = ret_arr[0];
	temp[1] = ret_arr[1];
	temp[0] = ret_arr[2];
	/*Really , whats going on?*/
	if(temp[0] & 0x40) // AM/PM
	{
		*str = ((temp[0] & 0x0F)+(((temp[0] >> 4) & 0x01) * 10));
		str++;
		hour = ((temp[0] >> 5) & 0x01);
		i = 1;
		while(i < 3)
		{
			*str = ((temp[i] & 0x0F)+((temp[i] >> 4) * 10));
			str++;
			i++;
		}
	}
	else // 24
	{
		i = 0;
		while(i < 3)
		{
			*str = ((temp[i] & 0x0F)+((temp[i] >> 4) * 10));
			str++;
			i++;
		}
		hour = DS3231_GET_24;
	}
	return hour;
}
static void ds3231_write_time(uint8_t h1224, uint8_t hours, uint8_t minutes, uint8_t seconds){
	i2c_send_byte(DS_ADDR, (0x1000 | DS3231_SECONDS), ds3231_byte(seconds));
	i2c_send_byte(DS_ADDR, (0x1000 | (DS3231_SECONDS + 1)), ds3231_byte(minutes));
	if(h1224 == DS3231_24)
	{
		i2c_send_byte(DS_ADDR, (0x1000 | (DS3231_SECONDS + 2)), ds3231_byte(hours) & DS3231_24);
	}
	else
	{
		if(h1224 == DS3231_AM)
		{
			if(hours > 12) hours -= 12;
			i2c_send_byte(DS_ADDR, (0x1000 | (DS3231_SECONDS + 2)), (ds3231_byte(hours) & DS3231_AM) | DS3231_HOURS_12);
		}
		else
		{
			if(hours > 12) hours -= 12;
			i2c_send_byte(DS_ADDR, (0x1000 | (DS3231_SECONDS + 3)), ds3231_byte(hours)| DS3231_PM);
		}
	}
}
static void ds3231_read_date(uint8_t *str){
	uint8_t temp[3];
	uint8_t i = 0;
	i2c_read_arr(DS_ADDR, (0x1000 | DS3231_DAY), 4);
	temp[0] = ret_arr[0];
	temp[1] = ret_arr[1];
	temp[2] = ret_arr[2];
	*str = ret_arr[3];
	str++;
	while(i < 4)
	{
		*str = ((temp[i] & 0x0F)+((temp[i] >> 4) * 10));
		str++;
		i++;
	}
}
static void ds3231_write_data(uint8_t date, uint8_t day, uint8_t month, uint8_t year){
	i2c_send_byte(DS_ADDR, (0x1000 | DS3231_DAY), ds3231_byte(date));
	i2c_send_byte(DS_ADDR, (0x1000 | (DS3231_DAY + 1)), ds3231_byte(day));
	i2c_send_byte(DS_ADDR, (0x1000 | (DS3231_DAY + 2)), ds3231_byte(month));
	i2c_send_byte(DS_ADDR, (0x1000 | (DS3231_DAY + 2)), ds3231_byte(year));
}
/*We need to stop SQW*/
static void ds3231_sqw_on(uint8_t rs){
	uint8_t temp = 0;
	temp = ds3231_read_reg(DS3231_CONTROL);
	temp &= 0xA0;
	temp |= ((1 << DS3231_BBSQW) | rs);
	ds3231_write_reg(DS3231_CONTROL, temp);
}
void shiftout(uint8_t type, uint8_t data){ //type: 0 goes for data, 1 goes for command
	if (type)
	PORTD &= 0b10111111; //Really?
	else
	PORTD |= 0b01000000;
	
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
}
/*Display reset, looks weird*/
static void lcd_res(){
			PORTD|=0b10000000;
			_delay_ms(1);
			PORTD&=0b01111111;
			_delay_ms(10);
			PORTD|=0b10000000;
			for (uint8_t u = 0; u < sizeof(res_seq) - 1; u++){
				shiftout(COM, eeprom_read_byte(&res_seq[u]));
			}
			for(uint16_t u = 0; u < 1024; u++){
				shiftout(DATA, 0x00);
			}
			shiftout(COM, eeprom_read_byte(&res_seq[sizeof(res_seq) - 2]));
}
/*Goes to page. 4-byte end address|4-byte start address*/
static void goto_page(uint8_t page){
	shiftout(COM, 0x22);
	shiftout(COM, page & 0x0F);
	shiftout(COM, (page >> 4) & 0x0F);
}
/*Goes to x-dim. 4-byte end address|4-byte start address*/
static void goto_x(uint8_t shift){
	shiftout(COM, 0x21);
	shiftout(COM, shift & 0x0F);
	shiftout(COM, (shift >> 4) & 0x0F);
}
/*Puts a string to lcd with small amount of parameters*/
static uint8_t word_out(uint8_t *param, uint8_t *input, uint8_t len){
	/*3 param: 
	page: 4-byte end address|4-byte start address
	x-cord: 4-byte end address|4-byte start address
	control: 0 for no word shift control, 1 for proper shift
	*/
	uint8_t width = 0;
	uint8_t curr_page = 0x0F & *param;
	goto_page(*(param));
	goto_x(*(param + 1));
	for (uint8_t w = 0; w < len; w++){
		if(!(input[w] + 1)){
			for(uint8_t q = 0; q < 5; q++)
				shiftout(DATA, 0x00);
			continue;
			width += 5;
		} else {
			uint8_t sign = 0;
			uint8_t q;
			for (q = w; q < len; q++){
				if (!(input[q] + 1))
					if (q > width){
						if (curr_page == 7){
							sign = 1;
						} else {
							curr_page++;
						}
					}
					break;
				}
			
			}
		i2c_read_arr(EEP_ADDR, (uint16_t) *(input + w) * 5, 5);
		for (uint16_t r = 0; r < 5; r++){
			shiftout(DATA, ret_arr[r]);
		}
		shiftout(DATA, 0x00);
		width += 6;
	}
}
/*what?*/




