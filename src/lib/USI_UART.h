#pragma once

#define BAUDRATE 38400
#define TIMER_PRESCALER 1

// Must be power of 2
#define UART_RX_BUFFER_SIZE 32
#define UART_TX_BUFFER_SIZE 32

unsigned char Bit_Reverse(unsigned char);
void USI_UART_Flush_Buffers(void);
void USI_UART_Initialise_Receiver(void);
void USI_UART_Initialise_Transmitter(void);
void USI_UART_Deactivate(void);
void USI_UART_Transmit_Byte(unsigned char);
void USI_UART_Transmit_CString(char*);
unsigned char USI_UART_Data_Transmitting(void);
void USI_UART_Receive_Enable(unsigned char);
unsigned char USI_UART_Receive_Byte(void);
unsigned char USI_UART_Copy_Receive_Buffer(char*, unsigned char);
unsigned char USI_UART_Data_In_Receive_Buffer(void);
