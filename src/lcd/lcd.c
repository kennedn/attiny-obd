#define F_CPU 8000000
#include <avr/io.h>
#include <util/delay.h>
#include "USI_TWI_Master.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>


#define ADDRESS 0x27
#define BACKLIGHT 0x08
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

#define EN 0x04
#define RW 0x02
#define RS_DATA 0x01
#define RS_INSR 0x00
#define LCD_DATA_MASK 0b00000000
#define LCD_CGRAM_MASK 0b01000000
#define LCD_DDRAM_MASK 0b10000000

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


static const uint16_t s_lcd_icons[] = {0x4, 0xa, 0xa, 0xe, 0xe, 0x1f, 0x1f, 0xe,     // thermometer
                                       0x4, 0x4, 0xa, 0xa, 0x11, 0x11, 0x11, 0xe};   // water droplet


static void lcd_write_icons() {
    // Custom 5x8 icons to send to module
    // Starting at first CG offset will cause character 1 to be accessible at \0, which causes issues for null terminated 
    //string functions. Quick hack is to lose a character and just start at offset 8 (\1)
    write2x4bits_and_pulse(LCD_CGRAM_MASK | 8, RS_INSR);

    // Write custom characters to LCD module, AC auto increments on each write
    for (int i = 0; i < COUNT_OF(s_lcd_icons); i++) {
        write2x4bits_and_pulse(LCD_DATA_MASK | s_lcd_icons[i], RS_DATA); // write each line of 5x8 character
    }
    // Return Home
    write2x4bits_and_pulse(0b00000010, RS_INSR);  // Set Address counter back to DDRAM
    _delay_us(2000); // Home command takes a long time
}


int main() {

  USI_TWI_Master_Initialise();
  srand(time(NULL));
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
  write2x4bits_and_pulse(0b00001100, RS_INSR);

  // Set cursor to right shift
  write2x4bits_and_pulse(0b00000110, RS_INSR);

  // Clear display
  write2x4bits_and_pulse(0b00000001, RS_INSR);
  _delay_us(2000); // Clear command takes a long time

  // // Return home
  // write2x4bits_and_pulse(0b00000010, RS_INSR);
  // _delay_us(2000); // Home command takes a long time

  lcd_write_icons();
  char lcd_data[40];
  snprintf(lcd_data, 40, "Coolant %c:", '\1');
  lcd_print(lcd_data);

  uint8_t temp1 = 86;
  uint8_t temp2 = 5;
  while(1) {
    int8_t dir = (rand() % 3) - 1;
    temp1 = (temp1 + dir) % 100;
    temp2 = rand() % 9;

    // Move to line 2
    write2x4bits_and_pulse(LCD_DDRAM_MASK | 0x40, RS_INSR);

    snprintf(lcd_data, 40, "%d.%01d%cC", temp1, temp2, '\xdf');
    // Pad line with spaces
    snprintf(lcd_data, 40, "%-40s", lcd_data);
    lcd_print(lcd_data);
    _delay_ms(1000);
  }

  return 0;
}

