#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

#include "elm327.h"
#include "lcd1602.h"

int main(void)
{
    lcd_initialise();
    char lcd_text[40];
    lcd_print_cstring("Coolant ");
    lcd_print_char('\1');
    lcd_print_char(':');

    elm327_initalise();
    elm327_send_command_and_wait("ATZ\r");   // Reset
    elm327_send_command_and_wait("ATE0\r");  // Echo off
    elm327_send_command_and_wait("ATTP6\r"); // Select protocol 6

    while (1)
    {
        elm327_initalise();
        elm327_send_command_and_wait("01051\r"); // (0105) Coolant temperature + 1 line expected
        elm327_retreive_cstring(5);

        elm327_deactivate();

        lcd_usi_initialise();

        // Move to line 2
        lcd_move(0x40);

        if (elm327_response_buffer[1] == 'T')
        {
            lcd_print_cstring("NO DATA");
        }
        else
        {
            long temp = strtol(elm327_response_buffer, NULL, 16);
            temp = temp - 40;
            ltoa(temp, lcd_text, 10);
            lcd_print_cstring(lcd_text);
            lcd_print_char('\xdf');
            lcd_print_char('C');
            lcd_print_cstring("   ");
        }
        _delay_ms(500);
    }
    return 0;
}
