#pragma once
void lcd_initialise(void);
void lcd_usi_initialise(void);
unsigned char lcd_print_cstring(const char*);
unsigned char lcd_print_char(const char);
unsigned char lcd_print_long(long);
void lcd_return_home(void);
void lcd_clear_display(void);
void lcd_move(unsigned char);

#define MAX_LONG_LEN 8
