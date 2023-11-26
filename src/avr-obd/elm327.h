#pragma once
#include "USI_UART.h"


void elm327_initalise(void);
void elm327_update_data(void);

char* elm327_get_prefix(void);
char* elm327_get_suffix(void);
long elm327_get_data(void);
long elm327_get_max_data(void);

// Debug
void elm327_send_command(char *s);
