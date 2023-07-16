#define F_CPU 8000000UL
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

int main(void) {

	DDRB  |= _BV(PB3) | _BV(PB4);                         // Configure PB3 and PB4 as output.
	
    while(1) {
        _delay_ms(20);
        PORTB = (PORTB & ~(_BV(PB4))) | _BV(PB3);         // Toggle PB3 On, PB4 Off
        _delay_ms(20);
        PORTB = (PORTB & ~(_BV(PB3))) | _BV(PB4);         // Toggle PB3 Off, PB4 On
    }

	return 0;
}
