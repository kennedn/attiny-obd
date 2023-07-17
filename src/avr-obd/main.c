#include <avr/io.h>
#include <util/delay.h>

#include "elm327.h"
#include "lcd1602.h"

int main(void) {

    lcd_initialise_full();
    elm327_initalise();
	
    elm327_send_command_and_wait("ATZ\r");        // Reset
    elm327_send_command_and_wait("ATE0\r");       // Echo off
    while(1) {
      elm327_initalise();
      elm327_send_command_and_wait("ATRV\r");     // Car battery voltage
      elm327_retreive_cstring(6);
      
      elm327_deactivate();

      lcd_initialise();

      // lcd_clear_display();
      lcd_return_home();
      lcd_print(elm327_response_buffer);
      _delay_ms(1000);

    }
	return 0;
}
