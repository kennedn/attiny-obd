
#include "elm327.h"
#include "eeprom.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "USI_UART.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((unsigned char)(!(sizeof(x) % sizeof(0 [x])))))
#define EEPROM_MAX_BASE 0x0
#define EEPROM_CONFIG 0xFF

char elm327_response_buffer[UART_RX_BUFFER_SIZE];

void elm327_send_command_and_wait(char*);
long elm327_unit_temp(long);
long elm327_unit_percent(long);
long elm327_unit_kph(long);


typedef struct ELM327_Command{
    char command[8];
    char prefix[12];
    char suffix[5];
    long data;
    long max_data;
    char bytes;
    long (*unit_func)(long);
} ELM327_Command;

ELM327_Command elm327_commands[] = {
    {
        .command = "01051\r",
        .prefix = "Coolant \x01" ": ",
        .suffix = "\xdf" "C",
        .data = 0,
        .max_data = 0,
        .bytes = 2,
        .unit_func = elm327_unit_temp
    },
    {
        .command = "012F1\r",
        .prefix = "Fuel Lvl: ",
        .suffix = "%",
        .data = 0,
        .max_data = 0,
        .bytes = 2,
        .unit_func = elm327_unit_percent
    },
    {
        .command = "010D1\r",
        .prefix = "Speed: ",
        .suffix = " kph",
        .data = 0,
        .max_data = 0,
        .bytes = 2,
        .unit_func = elm327_unit_kph
    }
};

unsigned char elm327_idx = 2;

void elm327_usi_initalise(void) {
    USI_UART_Initialise_Receiver();
    sei();  // Enable global interrupts
}

void elm327_initalise(void) {
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

long elm327_unit_temp(long x) {
    return (long)x - 40;
}

long elm327_unit_percent(long x) {
    return (long)((x * 100 + 128) >> 8);
}

long elm327_unit_kph(long x) {
    return (long)x;
}

void elm327_update_data() {
    elm327_usi_initalise();

    elm327_send_command_and_wait(NULL);
    // Required bytes + \x20 \xOD \xOD
    elm327_retrieve_cstring(elm327_commands[elm327_idx].bytes + 3);

    long raw_data = strtol(elm327_response_buffer, NULL, 16);
    long data = elm327_commands[elm327_idx].unit_func(raw_data);
    elm327_commands[elm327_idx].data = data;
    if (data > elm327_commands[elm327_idx].max_data) {
        elm327_commands[elm327_idx].max_data = data;
        eeprom_write(EEPROM_MAX_BASE, data);
    }

    elm327_deactivate();
}

char* elm327_get_prefix(void) {
    return elm327_commands[elm327_idx].prefix;
}

char* elm327_get_suffix(void) {
    return elm327_commands[elm327_idx].suffix;
}

long elm327_get_data(void) {
    return elm327_commands[elm327_idx].data;
}

long elm327_get_max_data(void) {
    return elm327_commands[elm327_idx].max_data;
}

void elm327_next_command(void) {
    elm327_idx = (elm327_idx + 1) % COUNT_OF(elm327_commands);
    //elm327_idx = (elm327_idx + 1) % 3;
    //elm327_idx = 0;
}

void elm327_previous_command(void) {
    elm327_idx = (elm327_idx + COUNT_OF(elm327_commands) - 1) % COUNT_OF(elm327_commands);
    //elm327_idx = (elm327_idx + 3 - 1) % 3;
    //elm327_idx = 2;
}

// Transmit a command on the elm327 uart and block until a '>' character is observed
// @param command Command to send to elm327
void elm327_send_command_and_wait(char *s) {
    if (s == NULL) { 
        USI_UART_Transmit_CString(elm327_commands[elm327_idx].command);
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
