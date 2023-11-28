
#include "elm327.h"
#include "storage.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "USI_UART.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define EEPROM_MAX_BASE 0x0
#define EEPROM_CONFIG 0xFF
#define ELM327_COMMAND_COUNT 3
#define ELM327_RECEIVE_TIMEOUT WDTO_250MS

char elm327_response_buffer[UART_RX_BUFFER_SIZE];

void elm327_send_command_and_wait(const char*);
long elm327_unit_temp(long);
long elm327_unit_percent(long);
long elm327_unit_kph(long);
char elm327_comp_max(long, long);
char elm327_comp_min(long, long);
void elm327_retrieve_stored_data(void);

long elm327_data;
long elm327_stored_data;

typedef struct ELM327_Command{
    const char *command;
    const char *prefix;
    const char *suffix;
    const char *stored_prefix;
    char bytes;
    long default_data;
    long (*unit_func)(long);
    char (*comp_func)(long, long);
} ELM327_Command;

ELM327_Command elm327_commands[] = {
    {
        .command = storage_command_0,
        .prefix = storage_prefix_0,
        .suffix = storage_suffix_0,
        .stored_prefix = storage_stored_prefix_0,
        .bytes = 2,
        .default_data = 0,
        .unit_func = elm327_unit_temp,
        .comp_func = elm327_comp_max
    },
    {
        .command = storage_command_1,
        .prefix = storage_prefix_1,
        .suffix = storage_suffix_1,
        .stored_prefix = storage_stored_prefix_1,
        .bytes = 2,
        .default_data = 255,
        .unit_func = elm327_unit_percent,
        .comp_func = elm327_comp_min
    },
    {
        .command = storage_command_2,
        .prefix = storage_prefix_2,
        .suffix = storage_suffix_2,
        .stored_prefix = storage_stored_prefix_2,
        .bytes = 2,
        .default_data = 0,
        .unit_func = elm327_unit_kph,
        .comp_func = elm327_comp_max
    }
};

unsigned char elm327_idx;

void elm327_usi_initalise(void) {
    USI_UART_Initialise_Receiver();
    sei();  // Enable global interrupts
}

void elm327_initalise(void) {
    elm327_send_command_and_wait(storage_command_a);    // Reset
    elm327_send_command_and_wait(storage_command_b);   // Echo off
    elm327_send_command_and_wait(storage_command_c);  // Select protocol 6
    USI_UART_Initialise_Receiver();
    elm327_idx = 0;
    elm327_retrieve_stored_data();
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

char elm327_comp_max(long d1, long d2) {
    return d1 > d2;
}

char elm327_comp_min(long d1, long d2) {
    return d1 < d2;
}

void elm327_update_data() {
    elm327_usi_initalise();

    elm327_send_command_and_wait(NULL);
    // Required bytes + \x20 \xOD \xOD
    elm327_retrieve_cstring(elm327_commands[elm327_idx].bytes + 3);

    long raw_data = strtol(elm327_response_buffer, NULL, 16);
    long data = elm327_commands[elm327_idx].unit_func(raw_data);
    elm327_data = data;
    if (elm327_commands[elm327_idx].comp_func(elm327_data, elm327_stored_data)) {
        elm327_stored_data = data;
        storage_write_long(elm327_idx, data);
    }

    elm327_deactivate();
}

const char* elm327_get_prefix(void) {
    return elm327_commands[elm327_idx].prefix;
}

const char* elm327_get_stored_prefix(void) {
    return elm327_commands[elm327_idx].stored_prefix;
}

const char* elm327_get_suffix(void) {
    return elm327_commands[elm327_idx].suffix;
}

long elm327_get_data(void) {
    return elm327_data;
}

long elm327_get_stored_data(void) {
    return elm327_stored_data;
}

void elm327_retrieve_stored_data(void) {
    long tmp = storage_read_long(elm327_idx);
    if (tmp == 0xFFFFFFFF) {
        elm327_stored_data = elm327_commands[elm327_idx].default_data;
    } else {
        elm327_stored_data = tmp;
    }
}

void elm327_next_command(void) {
    elm327_idx = (elm327_idx + 1) % ELM327_COMMAND_COUNT;
    elm327_retrieve_stored_data();
}

void elm327_previous_command(void) {
    elm327_idx = (elm327_idx + ELM327_COMMAND_COUNT - 1) % ELM327_COMMAND_COUNT;
    elm327_retrieve_stored_data();
}

// Transmit a command on the elm327 uart and block until a '>' character is observed
// @param command Command to send to elm327
void elm327_send_command_and_wait(const char *ptr) {
    if (ptr == NULL) { 
        ptr = elm327_commands[elm327_idx].command;
    } 
    if (ptr == NULL) {
        return;
    }

    storage_load_string(ptr);
    USI_UART_Transmit_CString(storage_string_buffer);

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
