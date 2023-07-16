#include <stdio.h>
#include <stdint.h>
#include <string.h>


#define UART_RX_BUFFER_SIZE        32    
#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1)
static unsigned char          UART_RxBuf[UART_RX_BUFFER_SIZE] = "K\r\r>M327 v1.4\r\r>ATE0\rOK\r\r>OK\r\r>O";
static volatile unsigned char UART_RxHead = 31;


void USI_UART_Copy_Receive_Buffer(char *buff, uint8_t n) {
    do {
        uint8_t buff_index = (UART_RxHead + UART_RX_BUFFER_SIZE - n) & UART_RX_BUFFER_MASK;
        printf("n: %d, i: %d, c: %x\n", n, buff_index, UART_RxBuf[buff_index]);
        *(buff++) = UART_RxBuf[buff_index];
    } while(--n);
    *buff = '\0';
}

int main(void) {
  uint8_t receive_buff_size = 4;
  char receive_buff[receive_buff_size];
  printf("hello :)\n");
  USI_UART_Copy_Receive_Buffer(receive_buff, receive_buff_size);
  for (uint8_t i=0; i < receive_buff_size + 1; i++) {
  	if (receive_buff[i] != '\r' && receive_buff[i] != '\0') {
            printf("%c", receive_buff[i]);
        } else {
            printf("%02x", receive_buff[i]);
        } 
  }
  printf("\n");
  return 0;
}
