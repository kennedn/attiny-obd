
#include "elm327.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#include "USI_UART.h"

void elm327_initalise(void) {
    USI_UART_Initialise_Receiver();
    sei();  // Enable global interrupts
}

void elm327_deactivate(void) {
    USI_UART_Deactivate();
    cli();
}
// Copies n response bytes as a cstring into the elm327 response buffer
// @param n Number of bytes to retreive
void elm327_retreive_cstring(unsigned char n) {
    USI_UART_Copy_Receive_Buffer(elm327_response_buffer, n);
}

// Transmit a command on the elm327 uart and block until a '>' character is observed
// @param command Command to send to elm327
void elm327_send_command_and_wait(char *command) {
    USI_UART_Transmit_CString(command);  // Reset
    while (USI_UART_Receive_Byte() != '>')
        ;
}