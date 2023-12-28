#pragma once
void lcd_initialise(void);
void lcd_usi_initialise(void);
unsigned char lcd_print_cstring(const char*);
unsigned char lcd_print_ptr(const char*);
unsigned char lcd_print_padding(unsigned char count);
unsigned char lcd_print_char(char c);
void lcd_return_home(void);
void lcd_clear_display(void);
void lcd_move(unsigned char);
