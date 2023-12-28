#pragma once
#include <avr/pgmspace.h>

#define STORAGE_DOT_SLOT 0
#define STORAGE_IDX_SLOT 1
#define STORAGE_COMMAND_OFFSET_SLOT 2

extern const char storage_alt_command_0[] PROGMEM;
extern const char storage_alt_string_0[] PROGMEM;


extern const char storage_no_data[] PROGMEM;

extern const char storage_command_a[] PROGMEM;
extern const char storage_command_b[] PROGMEM;
extern const char storage_command_c[] PROGMEM;

extern const char storage_command_0[] PROGMEM;
extern const char storage_command_1[] PROGMEM;
extern const char storage_command_2[] PROGMEM;
extern const char storage_command_3[] PROGMEM;
extern const char storage_command_4[] PROGMEM;

extern const char storage_prefix_0[] PROGMEM;
extern const char storage_prefix_1[] PROGMEM;
extern const char storage_prefix_2[] PROGMEM;
extern const char storage_prefix_3[] PROGMEM;
extern const char storage_prefix_4[] PROGMEM;

extern const char storage_suffix_0[] PROGMEM;
extern const char storage_suffix_1[] PROGMEM;
extern const char storage_suffix_2[] PROGMEM;
extern const char storage_suffix_3[] PROGMEM;

extern const char storage_stored_prefix_0[] PROGMEM;
extern const char storage_stored_prefix_1[] PROGMEM;
extern const char storage_stored_prefix_2[] PROGMEM;

extern const char storage_icon_0[] PROGMEM;
extern const char storage_icon_1[] PROGMEM;
extern const char storage_icon_2[] PROGMEM;
extern const char storage_icon_3[] PROGMEM;
extern const char storage_icon_4[] PROGMEM;

char storage_string_buffer[16];

void storage_write_long(unsigned char, long);
long storage_read_long(unsigned char);
void storage_load_string(const char*);
void storage_load_icon(const char * ptr, size_t size);
