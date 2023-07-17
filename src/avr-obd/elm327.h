#pragma once
#include "USI_UART.h"

char elm327_response_buffer[UART_RX_BUFFER_SIZE];

void elm327_initalise(void);
void elm327_deactivate(void);
void elm327_retreive_cstring(unsigned char);
void elm327_send_command_and_wait(char*);