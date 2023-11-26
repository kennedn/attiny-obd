#include <avr/io.h>
#include <util/delay.h>

#include "elm327.h"
#include "lcd1602.h"

#include <stdlib.h>

int main(void) {
    lcd_initialise();
    elm327_initalise();

    while (1) {
        elm327_update_data();

        // Setup USI for TWI communication
        lcd_usi_initialise();

        lcd_move(0x00);
        lcd_print_cstring(elm327_get_prefix());
        lcd_print_long(elm327_get_data());
        lcd_print_cstring(elm327_get_suffix());

        lcd_move(0x40);
        lcd_print_cstring("Max: ");
        lcd_print_long(elm327_get_max_data());
        lcd_print_cstring(elm327_get_suffix());

        _delay_ms(500);
        
    }
    return 0;
}
