#include <avr/io.h>
#include <util/delay.h>

#include "elm327.h"
#include "lcd1602.h"

#include <stdlib.h>
#define LEFT_PIN PB4
#define RIGHT_PIN PB3
#define DELAY_MS 500

unsigned int delay_ms = 0;

int main(void) {
    lcd_initialise();
    elm327_initalise();
    DDRB &= ~(_BV(LEFT_PIN) | _BV(RIGHT_PIN));  // Set pins as input
    PORTB |= _BV(LEFT_PIN); //enable pullup on input
    PORTB |= _BV(RIGHT_PIN); //enable pullup on input

    while (1) {
        elm327_update_data();

        // Setup USI for TWI communication
        lcd_usi_initialise();

        lcd_move(0x00);
        lcd_print_cstring(elm327_get_prefix());
        lcd_print_long(elm327_get_data());
        lcd_print_cstring(elm327_get_suffix());
        lcd_print_char(' ');
        lcd_print_char(' ');
        lcd_print_char(' ');
        lcd_print_char(' ');
        lcd_print_char(' ');
        lcd_print_char(' ');

        lcd_move(0x40);
        lcd_print_cstring("Max: ");
        lcd_print_long(elm327_get_max_data());
        lcd_print_cstring(elm327_get_suffix());
        lcd_print_char(' ');
        lcd_print_char(' ');
        lcd_print_char(' ');
        lcd_print_char(' ');
        lcd_print_char(' ');
        lcd_print_char(' ');

       while (delay_ms < DELAY_MS) {
            if (!(PINB & _BV(LEFT_PIN))) {  // Left button pressed
                elm327_previous_command();
                delay_ms += 100;
                _delay_ms(100);
            } else if (!(PINB & _BV(RIGHT_PIN))) {  // Right button pressed
                elm327_next_command();
                delay_ms += 100;
                _delay_ms(100);
            }
            delay_ms++;
            _delay_ms(1);
        }
        delay_ms = 0;
        
    }
    return 0;
}
