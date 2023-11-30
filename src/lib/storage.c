#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include "storage.h"
const char storage_alt_command_0[] PROGMEM = "04\r";                 // clear fault codes

const char storage_command_a[] PROGMEM = "ATZ\r";                    // reset
const char storage_command_b[] PROGMEM = "ATE0\r";                   // echo off
const char storage_command_c[] PROGMEM = "ATTP6\r";                  // select protocol 6 (e.g protocol used by corsa d 2008)

const char storage_command_0[] PROGMEM = "01051\r";                  // coolant temp
const char storage_command_1[] PROGMEM = "012F1\r";                  // fuel level
const char storage_command_2[] PROGMEM = "010D1\r";                  // speed
const char storage_command_3[] PROGMEM = "01021\r";                  // DTC freeze frame

const char storage_prefix_0[] PROGMEM = "\1 Coolant: ";
const char storage_prefix_1[] PROGMEM = "\2 Fuel Lvl: ";
const char storage_prefix_2[] PROGMEM = "\3 Speed: ";
const char storage_prefix_3[] PROGMEM = "\4 Fault: ";

const char storage_suffix_0[] PROGMEM = "\xdf" "C";
const char storage_suffix_1[] PROGMEM = "%";
const char storage_suffix_2[] PROGMEM = " kph";
const char storage_suffix_3[] PROGMEM = " ";

const char storage_stored_prefix_0[] PROGMEM = "  Max: ";
const char storage_stored_prefix_1[] PROGMEM = "  Min: ";
const char storage_stored_prefix_2[] PROGMEM = "  Max: ";
const char storage_stored_prefix_3[] PROGMEM = "  Last: ";

const char storage_icon_0[] PROGMEM = {0x4, 0xa, 0xa, 0xe, 0xe, 0x1f, 0x1f, 0xe};  // thermometer
const char storage_icon_1[] PROGMEM = {0x0, 0x4, 0xe, 0xe, 0x1f, 0x1f, 0x1f, 0xe};  // teardrop
const char storage_icon_2[] PROGMEM = {0x0, 0x14, 0xa, 0x5, 0xa, 0x14, 0x0, 0x0};  // racing stripes
const char storage_icon_3[] PROGMEM = {0xe, 0xe, 0xe, 0xe, 0x4, 0x0, 0xe, 0xe};    // warning

void storage_write_long(unsigned char slot, long data)
{
    eeprom_write_dword((uint32_t*)(4+slot*4), (uint32_t)data);
}

long storage_read_long(unsigned char slot)
{
    return eeprom_read_dword((uint32_t*)(4+slot*4));
}

void storage_load_string(const char * ptr) {
    strcpy_P(storage_string_buffer, ptr);
}

void storage_load_icon(const char * ptr, size_t size) {
    memcpy_P(storage_string_buffer, ptr, size);
}
