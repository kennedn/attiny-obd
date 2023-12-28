
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

#define ELM327_COMMAND_COUNT 5

char elm327_response_buffer[UART_RX_BUFFER_SIZE];

long elm327_unit_temp(long);
long elm327_unit_percent(long);

char elm327_comp_max(long, long);
char elm327_comp_min(long, long);
char elm327_comp_last(long, long);

char* elm327_print_long(long);
char* elm327_print_dtc(long);

void elm327_send_command_and_wait(const char*);
void elm327_setup_data(void);

long elm327_data;
long elm327_stored_data;

typedef struct ELM327_Command{
    const char *command;
    const char *prefix;
    const char *suffix;
    const char *stored_prefix;
    char chars;
    long default_data;
    char* (*print_func)(long);
    long (*unit_func)(long);
    char (*comp_func)(long, long);
} ELM327_Command;

ELM327_Command elm327_commands[] = {
    {
        .command = storage_command_0,                   // Coolant Temp
        .prefix = storage_prefix_0,
        .suffix = storage_suffix_0,
        .stored_prefix = storage_stored_prefix_0,
        .chars = 2,
        .default_data = 0,
        .print_func = elm327_print_long,
        .unit_func = elm327_unit_temp,
        .comp_func = elm327_comp_max
    },
    {
        .command = storage_command_1,                   // Fuel level
        .prefix = storage_prefix_1,
        .suffix = storage_suffix_1,
        .stored_prefix = storage_stored_prefix_1,
        .chars = 2,
        .default_data = 255,
        .print_func = elm327_print_long,
        .unit_func = elm327_unit_percent,
        .comp_func = elm327_comp_min
    },
    {
        .command = storage_command_2,                   // Speed
        .prefix = storage_prefix_2,
        .suffix = storage_suffix_2,
        .stored_prefix = storage_stored_prefix_0,
        .chars = 2,
        .default_data = 0,
        .print_func = elm327_print_long,
        .unit_func = NULL,
        .comp_func = elm327_comp_max
    },
    {
        .command = storage_command_3,                   // DTC
        .prefix = storage_prefix_3,
        .suffix = storage_suffix_3,
        .stored_prefix = storage_stored_prefix_2,
        .chars = 5,
        .default_data = 0,
        .print_func = elm327_print_dtc,
        .unit_func = NULL,
        .comp_func = elm327_comp_last
    },
    {
        .command = storage_command_4,                   // Engine load
        .prefix = storage_prefix_4,
        .suffix = storage_suffix_1,
        .stored_prefix = storage_stored_prefix_0,
        .chars = 2,
        .default_data = 0,
        .print_func = elm327_print_long,
        .unit_func = elm327_unit_percent,
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
    // Index stored at storage index 0
    elm327_idx = (unsigned char)storage_read_long(STORAGE_IDX_SLOT);

    // EEPROM has not been set before, so fallback to idx 0
    if (elm327_idx == 0xFF) {
        elm327_idx = 0;
    }
    elm327_setup_data();
    sei();  // Enable global interrupts
}


void elm327_deactivate(void)  {
    USI_UART_Deactivate();
    cli();
}
// Copies n response chars as a cstring into the elm327 response buffer
// @param n Number of chars to retrieve
unsigned char elm327_retrieve_cstring(unsigned char n) {
    return USI_UART_Copy_Receive_Buffer(elm327_response_buffer, n);
}

long elm327_unit_temp(long x) {
    return (long)x - 40;
}

// Use fixed point arithmetic to multiply by 100 and divide by 255
// Rounding is achieved by offsetting the value by 128 
// Shift by 2^8 (256) to approximate division by 255
long elm327_unit_percent(long x) {
    return (long)((x * 100 + 128) >> 8);
}

char elm327_comp_max(long d1, long d2) {
    return d1 > d2;
}

char elm327_comp_min(long d1, long d2) {
    return d1 < d2;
}

char elm327_comp_last(long d1, long d2) {
    return d1 != d2 && d1 != 0;
}

char* elm327_print_long(long d) {
    ltoa(d, elm327_response_buffer, 10);
    return elm327_response_buffer;
}

char* elm327_print_dtc(long d) {
    char systems[] = {'P','C', 'B', 'U'};
    elm327_response_buffer[0] = systems[d >> 14];
    // Pad with leading 0 if required
    if ((d & 0x3000) == 0) {
        elm327_response_buffer[1] = '0';
        ltoa(d & 0x3FFF, &(elm327_response_buffer[2]), 16);
    } else {
        ltoa(d & 0x3FFF, &(elm327_response_buffer[1]), 16);
    }
    return elm327_response_buffer;
}

void elm327_update_data() {
    elm327_usi_initalise();
    elm327_send_command_and_wait(NULL);
    // Retrieve valid alphanumeric characters from the last n bytes of the response data
    // Example data: 7E8 03 41 05 e1 \r\r
    // To retrieve a bytes worth of hex code (e1), you need to iterate over 5 chars, which gives us:
    // e1 \r\r
    // Spaces and CRs are stripped so the function actually returns:
    // e1
    if (!elm327_retrieve_cstring(elm327_commands[elm327_idx].chars + 3)) {
        elm327_data = 0xFFFFFFFF;
        elm327_deactivate();
        return; // Return early if non hex characters were detected in data
    }
    long raw_data = strtol(elm327_response_buffer, NULL, 16);

    long data;
    if (elm327_commands[elm327_idx].unit_func != NULL) {
        data = elm327_commands[elm327_idx].unit_func(raw_data);
    } else {
        data = raw_data;
    }

    elm327_data = data;
    if (elm327_commands[elm327_idx].comp_func(elm327_data, elm327_stored_data)) {
        elm327_stored_data = data;
        // Stored slot data starts at idx 1
        storage_write_long(STORAGE_COMMAND_OFFSET_SLOT + elm327_idx, data);
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

char* elm327_get_data(void) {
    return elm327_commands[elm327_idx].print_func(elm327_data);
}

char* elm327_get_stored_data(void) {
    return elm327_commands[elm327_idx].print_func(elm327_stored_data);
}

const char* elm327_get_alt_string(void) {
    return storage_alt_string_0;
}


unsigned char elm327_has_alt(void) {
    return elm327_idx == 3;
}

unsigned char elm327_has_data(void) {
    return elm327_data != 0xFFFFFFFF;
}

void elm327_send_alt_command(void) {
    elm327_usi_initalise();
    elm327_send_command_and_wait(storage_alt_command_0);
    elm327_deactivate();
}

void elm327_setup_data(void) {
    // Ensure residual bits will be zero 
    elm327_data = 0xFFFFFFFF;
    elm327_stored_data = 0;

    // Stored slot data starts at idx 1
    long tmp = storage_read_long(STORAGE_COMMAND_OFFSET_SLOT + elm327_idx);

    // EEPROM has not been set before, so fallback to default data
    if (tmp == 0xFFFFFFFF) {
        elm327_stored_data = elm327_commands[elm327_idx].default_data;
    } else {
        elm327_stored_data = tmp;
    }

    // Store idx in eeprom
    storage_write_long(STORAGE_IDX_SLOT, (long)elm327_idx);
}

void elm327_next_command(void) {
    elm327_idx = (elm327_idx + 1) % ELM327_COMMAND_COUNT;
    elm327_setup_data();
}

void elm327_previous_command(void) {
    elm327_idx = (elm327_idx + ELM327_COMMAND_COUNT - 1) % ELM327_COMMAND_COUNT;
    elm327_setup_data();
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
    USI_UART_Transmit_CString(s);
    _delay_ms(20);
}
