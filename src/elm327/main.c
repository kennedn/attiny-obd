#define F_CPU 8000000UL
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "USI_UART.h"

char uart_response_buffer[UART_RX_BUFFER_SIZE];

int main(void) {

    USI_UART_Initialise_Receiver();
    sei();	// Enable global interrupts
	// _delay_ms(1000);0
	DDRB  |= _BV(PB3) | _BV(PB4);                         // Configure PB3 as output.
    // PORTB ^= _BV(PB4);
	
	// for (;;) {		
		// if (USI_UART_Data_In_Receive_Buffer()) {	// Check if there is data in the Rx buffer
		    // unsigned char b = USI_UART_Receive_Byte();	// Receive a byte.
        // _delay_ms(10000);	// Wait a while for all the data to come
        // USI_UART_Receive_Enable(0);
        USI_UART_Transmit_CString("ATZ\r"); // Reset 
        while(USI_UART_Receive_Byte() != '>');

        USI_UART_Transmit_CString("ATE0\r"); // Echo off
        while(USI_UART_Receive_Byte() != '>');

        USI_UART_Transmit_CString("ATRV\r"); // Battery Voltage
        while(USI_UART_Receive_Byte() != '>');
        USI_UART_Copy_Receive_Buffer(uart_response_buffer, 6);
        USI_UART_Deactivate();
        
        USI_UART_Transmit_CString(uart_response_buffer);
        // } 
        _delay_ms(1000);
	return 0;
}
