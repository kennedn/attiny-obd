#define F_CPU 8000000
#include "usi_uart.h"
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <string.h>

int main( void ) {
    const char echo_str[] = "ATZ\r";

    unsigned char i;

    uuart_flush_buffers();
    uuart_init_receiver();

    // Enable global interrupts
    cli();
    _delay_ms(1000);
    for ( i = 0; i < strlen( echo_str ); i++ ) {
        uuart_tx_byte( (unsigned char)echo_str[i] );
    }
    // while(1) {
    //     if ( uuart_data_in_rx_buffer() ) {
    //         uuart_tx_byte( uuart_rx_byte() );
    //     }
    //     // sleep_enable();
    // }

    return 0;
}
