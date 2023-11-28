
#include "elm327.h"
#include "eeprom.h"

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
long elm327_comp_max(long, long);
long elm327_comp_min(long, long);

long elm327_data;
long elm327_eeprom_data;

const char flash_command1[] PROGMEM = "01051\r";
const char flash_prefix1[] PROGMEM = "Coolant \x01" ": ";
const char flash_suffix1[] PROGMEM = "\xdf" "C";
const char flash_command2[] PROGMEM = "012F1\r";
const char flash_prefix2[] PROGMEM = "Fuel Lvl: ";
const char flash_suffix2[] PROGMEM = "%";
const char flash_command3[] PROGMEM = "010D1\r";
const char flash_prefix3[] PROGMEM = "Speed: ";
const char flash_suffix3[] PROGMEM = " kph";

const char flash_command4[] PROGMEM = "ATZ\r";
const char flash_command5[] PROGMEM = "ATE0\r";
const char flash_command6[] PROGMEM = "ATTP6\r";

typedef struct ELM327_Command{
    const char *command;
    const char *prefix;
    const char *suffix;
    char bytes;
    long (*unit_func)(long);
    char (*comp_func)(long, long);
} ELM327_Command;

ELM327_Command elm327_commands[] = {
    {
        .command = flash_command1,
        .prefix = flash_prefix1,
        .suffix = flash_suffix1,
        .bytes = 2,
        .unit_func = elm327_unit_temp
    },
    {
        .command = flash_command2,
        .prefix = flash_prefix2,
        .suffix = flash_suffix2,
        .bytes = 2,
        .unit_func = elm327_unit_percent
    },
    {
        .command = flash_command3,
        .prefix = flash_prefix3,
        .suffix = flash_suffix3,
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
    elm327_send_command_and_wait(flash_command4);    // Reset
    elm327_send_command_and_wait(flash_command5);   // Echo off
    elm327_send_command_and_wait(flash_command6);  // Select protocol 6
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

long elm327_comp_max(long d1, long d2) {
    return d1 > d2;
}

long elm327_comp_min(long d1, long d2) {
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
    if (data > elm327_eeprom_data) {
        elm327_eeprom_data = data;
        eeprom_write(EEPROM_MAX_BASE, data);
    }

    elm327_deactivate();
}

const char* elm327_get_prefix(void) {
    return elm327_commands[elm327_idx].prefix;
}

const char* elm327_get_suffix(void) {
    return elm327_commands[elm327_idx].suffix;
}

long elm327_get_data(void) {
    return elm327_data;
}

long elm327_get_eeprom_data(void) {
    return elm327_eeprom_data;
}

void elm327_next_command(void) {
    elm327_idx = (elm327_idx + 1) % ELM327_COMMAND_COUNT;
}

void elm327_previous_command(void) {
    elm327_idx = (elm327_idx + ELM327_COMMAND_COUNT - 1) % ELM327_COMMAND_COUNT;
}

// Transmit a command on the elm327 uart and block until a '>' character is observed
// @param command Command to send to elm327
void elm327_send_command_and_wait(const char *s) {
    char buffer[20];
    if (s == NULL) { 
        s = elm327_commands[elm327_idx].command;
    } 
    if (s != NULL) {
        strcpy_P(buffer, s);
        USI_UART_Transmit_CString(buffer);
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
