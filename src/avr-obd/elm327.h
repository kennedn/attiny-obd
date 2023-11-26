#pragma once
#include "USI_UART.h"

char elm327_response_buffer[UART_RX_BUFFER_SIZE];

typedef struct ELM327_Command{
    char command[8];
    char prefix[20];
    char suffix[20];
    char data;
    char max_data;
    long (*converter)();
} ELM327_Command;

ELM327_Command elm327_command;

void elm327_initalise(void);
void elm327_update_data(void);

// Debug
void elm327_send_command(char *s);
