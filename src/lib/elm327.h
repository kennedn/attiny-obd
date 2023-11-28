#pragma once
#include "USI_UART.h"


void elm327_initalise(void);
void elm327_update_data(void);

const char* elm327_get_prefix(void);
const char* elm327_get_suffix(void);
long elm327_get_data(void);
long elm327_get_eeprom_data(void);
void elm327_next_command(void);
void elm327_previous_command(void);

// Debug
void elm327_send_command(char *s);
