#include "USI_TWI_Master.h"
#include "lcd1602.h"

#include <stdlib.h>

#define ADDRESS 0x27
#define BACKLIGHT 0x08

#define EN 0x04
#define RW 0x02
#define RS_DATA 0x01
#define RS_INSR 0x00
#define LCD_DATA_MASK 0b00000000
#define LCD_CGRAM_MASK 0b01000000
#define LCD_DDRAM_MASK 0b10000000
#define COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((unsigned char)(!(sizeof(x) % sizeof(0 [x])))))

static const uint16_t s_lcd_icons[] = {0x4, 0xa, 0xa, 0xe, 0xe, 0x1f, 0x1f, 0xe};  // thermometer

void write8bits(unsigned char data) {
    char _data[2] = {
        ADDRESS << 1,
        data | BACKLIGHT};
    USI_TWI_Start_Transceiver_With_Data((unsigned char *)_data, 2);
}

void pulse_enable_bit(unsigned char data) {
    write8bits(data | EN);
    _delay_us(1);

    write8bits(data & ~EN);
    _delay_us(50);
}

void write2x4bits_and_pulse(unsigned char data, unsigned char reg_select) {
    for (unsigned char i = 0; i < 2; i++) {
        unsigned char _data = (data << (i * 4)) & 0XF0;
        write8bits(_data | reg_select);
        pulse_enable_bit(_data | reg_select);
    }
}

unsigned char lcd_print_cstring(const char *data) {
    unsigned char i = 0;
    do {
        write2x4bits_and_pulse(*(data++), RS_DATA);
        i++;
    } while (*data);
    return i;
}

unsigned char lcd_print_char(const char c) {
    write2x4bits_and_pulse(c, RS_DATA);
    return 1;
}

unsigned char lcd_print_long(long l) {
    char tmp[MAX_LONG_LEN + 1]; // + 1 for null terminator
    ltoa(l, tmp, 10);
    return lcd_print_cstring(tmp);
}

void lcd_return_home(void) {
    // Return home
    write2x4bits_and_pulse(0b00000010, RS_INSR);
    _delay_us(2000);  // Home command takes a long time
}

void lcd_clear_display(void) {
    // Clear display
    write2x4bits_and_pulse(0b00000001, RS_INSR);
    _delay_us(2000);  // Clear command takes a long time
}

static void lcd_write_icons() {
    // Starting at first CG offset will cause character 1 to be accessible at \0, which causes issues for null terminated
    // string functions. Quick hack is to lose a character and just start at offset 8 (\1)
    write2x4bits_and_pulse(LCD_CGRAM_MASK | 8, RS_INSR);

    // Write custom characters to LCD module, AC auto increments on each write
    for (int i = 0; i < COUNT_OF(s_lcd_icons); i++) {
        write2x4bits_and_pulse(LCD_DATA_MASK | s_lcd_icons[i], RS_DATA);  // write each line of 5x8 character
    }
    // Return Home (Sets pointer back to DDRAM)
    lcd_return_home();
}

void lcd_move(unsigned char location) {
    write2x4bits_and_pulse(LCD_DDRAM_MASK | location, RS_INSR);
}

void lcd_usi_initialise(void) {
    USI_TWI_Master_Initialise();
}

void lcd_initialise(void) {
    USI_TWI_Master_Initialise();

    _delay_ms(50);

    // Initialise 8 bit mode, attempt 1
    write8bits(0x03 << 4);
    pulse_enable_bit(0x03 << 4);
    _delay_us(4500);  // wait for more than 4.1ms

    // Initialise 8 bit mode, attempt 2
    write8bits(0x03 << 4);
    pulse_enable_bit(0x03 << 4);
    _delay_us(150);  // wait for more than 40us

    // Initialise 4 bit mode
    write8bits(0x02 << 4);
    pulse_enable_bit(0x02 << 4);

    // Initialise 4 bit mode, 2 line mode, 5 x 8 font
    write2x4bits_and_pulse(0b00101000, RS_INSR);

    // Display on, cursor on, cursor blink
    write2x4bits_and_pulse(0b00001100, RS_INSR);

    // Set cursor to right shift
    write2x4bits_and_pulse(0b00000110, RS_INSR);

    lcd_clear_display();
    // lcd_return_home();
    lcd_write_icons();
}
