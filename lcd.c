#define F_CPU 8000000
#include <avr/io.h>
#include <util/delay.h>
#include "USI_TWI_Master.h"
#include <stdlib.h>
#include <stdio.h>
// #include <string.h>
//#include <inttypes.h>


#define ADDRESS 0x27
#define BACKLIGHT 0x08

#define EN 0x04
#define RW 0x02
#define RS_DATA 0x01
#define RS_INSR 0x00

void write(uint8_t data) {
  char _data[2] = { 
    ADDRESS << 1,
    data | BACKLIGHT
  };
  USI_TWI_Start_Transceiver_With_Data(_data, 2);
}

void pulse_enable_bit(uint8_t data) {
  write(data | EN);
  _delay_us(1);

  write(data & ~EN);
  _delay_us(50);
}

void write2x4bits_and_pulse(uint8_t data, uint8_t reg_select) {
  for (uint8_t i=0; i < 2; i++) {
    uint8_t _data = (data << (i * 4)) & 0XF0;
    write(_data | reg_select);
    pulse_enable_bit(_data | reg_select);
  }
}

void lcd_print(const char *data) {
  do {
    write2x4bits_and_pulse(*(data++), RS_DATA);
  } while (*data);
}

int main() {

  USI_TWI_Master_Initialise();

  //Follow initialisation process according to the hitachi HD44780 datasheet
  // figure 24, pg 46
  
  // Pinout for transfer goes:
  // DB7 - DB6 - DB5 - DB4 - BL- E - RW - RS

  _delay_ms(50);

  // Initialise 8 bit mode, attempt 1
  write(0x03 << 4);
  pulse_enable_bit(0x03 << 4);
  _delay_us(4500); // wait for more than 4.1ms


  // Initialise 8 bit mode, attempt 2
  write(0x03 << 4);
  pulse_enable_bit(0x03 << 4);
  _delay_us(150); // wait for more than 40us

  // Initialise 4 bit mode
  write(0x02 << 4);
  pulse_enable_bit(0x02 << 4);

  // Initialise 4 bit mode, 2 line mode, 5 x 8 font
  write2x4bits_and_pulse(0b00101000, RS_INSR);

  // Display on, cursor on, cursor blink
  write2x4bits_and_pulse(0b00001111, RS_INSR);

  // Set cursor to right shift
  write2x4bits_and_pulse(0b00000110, RS_INSR);

  // Clear display
  write2x4bits_and_pulse(0b00000001, RS_INSR);
  _delay_us(2000); // Clear command takes a long time

  // Return home
  write2x4bits_and_pulse(0b00000010, RS_INSR);
  _delay_us(2000); // Home command takes a long time
  char lcd_data[40];
  snprintf(lcd_data, 41, "%s", "Monkey :)");
  lcd_print(lcd_data);

  return 0;
}

