#include <avr/io.h>
#include <util/delay.h>

#include "elm327.h"
#include "lcd1602.h"

#include <stdlib.h>

int main(void) {
    // Setup USI for TWI communication, initialise display and install icons
    lcd_initialise();
    _delay_ms(500);
    elm327_initalise();

    char lcd_text[16];
    //lcd_print_cstring("Coolant ");
    //lcd_print_char('\1');
    //lcd_print_char(':');

    while (1) {
        elm327_update_data();

        // Setup USI for TWI communication
        lcd_usi_initialise();

        lcd_move(0x00);
        lcd_print_cstring(elm327_command.prefix);

        lcd_move(0x40);
        ltoa(elm327_command.data, lcd_text, 10);
        lcd_print_cstring(lcd_text);
        lcd_print_cstring(elm327_command.suffix);

        _delay_ms(500);
        
    }
    return 0;
}
