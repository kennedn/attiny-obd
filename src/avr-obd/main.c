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
    PORTB |= _BV(LEFT_PIN) | _BV(RIGHT_PIN); //enable pullup on input buttons

    while (1) {
       // Check for button press, debounce and break if detected
       while (delay_ms < DELAY_MS) {
            if (!(PINB & _BV(LEFT_PIN))) {  // Left button pressed
                elm327_previous_command();
                break;
            } else if (!(PINB & _BV(RIGHT_PIN))) {  // Right button pressed
                elm327_next_command();
                break;
            }
            delay_ms++;
            _delay_ms(1);
        }

        elm327_update_data();

        // Setup USI for TWI communication
        lcd_usi_initialise();

        lcd_move(0x00);
        lcd_print_cstring(elm327_get_prefix());
        lcd_print_long(elm327_get_data());
        lcd_print_cstring(elm327_get_suffix());
        lcd_print_padding(6);

        lcd_move(0x40);
        //lcd_print_cstring("Max: ");
        lcd_print_long(elm327_get_eeprom_data());
        lcd_print_cstring(elm327_get_suffix());
        lcd_print_padding(6);
   
        // Delay for any remaining time in the case that a button press broke the prior loop early 
        while (delay_ms < DELAY_MS) {
            delay_ms++;
           _delay_ms(1);
        }
        delay_ms = 0;
    }
    return 0;
}
