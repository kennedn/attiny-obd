#include <avr/io.h>
#include <util/delay.h>
#include <avr/wdt.h>

#include "lib/elm327.h"
#include "lib/lcd1602.h"
#include "lib/storage.h"

#include <stdlib.h>
#define LEFT_PIN PB4
#define RIGHT_PIN PB3
#define DELAY_MS 500

unsigned int delay_ms = 0;

void button_handler(unsigned char pin, void (*callback)(void)) {
    unsigned int depress_target = delay_ms + 1000;
    lcd_usi_initialise();
    while(1) {
       wdt_reset();
        delay_ms++;
        _delay_ms(1);
        if ((PINB & _BV(pin))) {  // button depressed
            callback();
            break;
        }
        if (delay_ms < depress_target) {
            continue;
        }

        if (elm327_has_alt()) {
            lcd_move(0x00);
            lcd_print_ptr(elm327_get_prefix());
            lcd_print_ptr(elm327_get_alt_string());
            elm327_send_alt_command();
        }
        // Wait for depression before exiting
        while(!(PINB & _BV(pin))) {
            wdt_reset();
            _delay_ms(1);
        }
        break;
    }
}

int main(void) {
    long reset_count = storage_read_long(STORAGE_DOT_SLOT);
    if (reset_count == 0xFFFFFFFF) {
       reset_count = 0 ;
    }
    storage_write_long(STORAGE_DOT_SLOT, (reset_count + 1) % 3);
    wdt_reset();
    wdt_enable(WDTO_1S);
    lcd_initialise();
    lcd_move(0x00);
    lcd_print_ptr(storage_no_data);
    for (long i=0; i < reset_count + 1; i++) {
        lcd_print_char('.');
    }

    elm327_initalise();
    PORTB |= _BV(LEFT_PIN) | _BV(RIGHT_PIN); //enable pullup on input buttons

    while (1) {
        wdt_reset();
        // Check for button press, debounce and break if detected
        while (delay_ms < DELAY_MS) {
            if (!(PINB & _BV(LEFT_PIN))) {
                button_handler(LEFT_PIN, elm327_previous_command);
                break;
            } else if (!(PINB & _BV(RIGHT_PIN))) {
                button_handler(RIGHT_PIN, elm327_next_command);
                break;
            }
            delay_ms++;
            _delay_ms(1);
        }

        elm327_update_data();
        
        // Setup USI for TWI communication
        lcd_usi_initialise();

        lcd_move(0x00);
        lcd_print_ptr(elm327_get_prefix());
        if (elm327_has_data()) {
            lcd_print_cstring(elm327_get_data());
        } else {
            lcd_print_char('?');
        }
        lcd_print_ptr(elm327_get_suffix());
        lcd_print_padding(6);

        lcd_move(0x40);
        lcd_print_ptr(elm327_get_stored_prefix());
        lcd_print_cstring(elm327_get_stored_data());
        lcd_print_ptr(elm327_get_suffix());
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
