
#include "elm327.h"
#include "eeprom.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "USI_UART.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define EEPROM_MAX_BASE 0x0
#define EEPROM_CONFIG 0xFF

char elm327_response_buffer[UART_RX_BUFFER_SIZE];

typedef struct ELM327_Command{
    char command[8];
    char prefix[20];
    char suffix[20];
    long data;
    long max_data;
    unsigned char raw_data;
    unsigned char raw_max_data;
    long (*converter)(unsigned char);
} ELM327_Command;

ELM327_Command elm327_command;

void elm327_send_command_and_wait(char*);
long elm327_convert_temperature(unsigned char);

void elm327_usi_initalise(void) {
    USI_UART_Initialise_Receiver();
    sei();  // Enable global interrupts
}

void elm327_initalise(void) {
    strncpy(elm327_command.command, "01051\r", 16);
    strncpy(elm327_command.prefix, "Coolant \1" ": ", 16);
    strncpy(elm327_command.suffix, "\xdf" "C              ", 16);
    elm327_command.raw_max_data = eeprom_read(EEPROM_MAX_BASE);
    elm327_command.converter = &elm327_convert_temperature;

    elm327_send_command_and_wait("ATZ\r");    // Reset
    elm327_send_command_and_wait("ATE0\r");   // Echo off
    elm327_send_command_and_wait("ATTP6\r");  // Select protocol 6
    USI_UART_Initialise_Receiver();
    sei();  // Enable global interrupts
}


void elm327_deactivate(void)  {
    USI_UART_Deactivate();
    cli();
}
// Copies n response bytes as a cstring into the elm327 response buffer
// @param n Number of bytes to retrieve
void elm327_retrieve_cstring(unsigned char n) {
    USI_UART_Copy_Receive_Buffer(elm327_response_buffer, n);
}

// Converts a temperature string inline by rebasing it to -40
// @param s Temperature string to rebase inline
long elm327_convert_temperature(unsigned char x) {
    return (long)x - 40;
}

void elm327_update_data() {
    elm327_usi_initalise();

    elm327_send_command_and_wait(NULL);
    elm327_retrieve_cstring(5);

    unsigned char raw_data = (unsigned char)strtol(elm327_response_buffer, NULL, 16);
    long data = elm327_command.converter(raw_data);
    elm327_command.raw_data = raw_data;
    elm327_command.data = data;
    if (raw_data > elm327_command.raw_max_data) {
        elm327_command.raw_max_data = raw_data;
        elm327_command.max_data = data;
        eeprom_write(EEPROM_MAX_BASE, raw_data);
    }

    elm327_deactivate();
}

char* elm327_get_prefix(void) {
    return elm327_command.prefix;
}

char* elm327_get_suffix(void) {
    return elm327_command.suffix;
}

long elm327_get_data(void) {
    return elm327_command.data;
}

long elm327_get_max_data(void) {
    return elm327_command.max_data;
}

// Transmit a command on the elm327 uart and block until a '>' character is observed
// @param command Command to send to elm327
void elm327_send_command_and_wait(char *s) {
    if (s == NULL) { 
        USI_UART_Transmit_CString(elm327_command.command);
    } else {
        USI_UART_Transmit_CString(s);
    }
    while (USI_UART_Receive_Byte() != '>');
}

// Transmit a command on the elm327 uart with no blocking
// @param command Command to send to elm327
void elm327_send_command(char *s) {
    elm327_usi_initalise();
    USI_UART_Transmit_CString(s);
    _delay_ms(20);
    elm327_deactivate();
}
