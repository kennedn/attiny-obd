#pragma once
#include <avr/io.h>
#include <util/delay.h>

#define TWI_FAST_MODE

#ifdef TWI_FAST_MODE                            // TWI FAST mode timing limits. SCL = 100-400kHz
#define T2_TWI 1.55                             // >1.3us, + 0.25us
#define T4_TWI 0.85                             // >0.6us, + 0.25us

#else                                           // TWI STANDARD mode timing limits. SCL <= 100kHz
#define T2_TWI 4.95                             // >4.7us, + 0.25us
#define T4_TWI 4.25                             // >4.0us, + 0.25us
#endif

#define TWI_READ_BIT 0 // Bit position for R/W bit in "address byte".
#define TWI_NACK_BIT 0 // Bit position for (N)ACK bit.

#define USI_TWI_NO_DATA 0x00           // Transmission buffer is empty
#define USI_TWI_DATA_OUT_OF_BOUND 0x01 // Transmission buffer is outside SRAM space
#define USI_TWI_UE_START_CON 0x02      // Unexpected Start Condition
#define USI_TWI_UE_STOP_CON 0x03       // Unexpected Stop Condition
#define USI_TWI_UE_DATA_COL 0x04       // Unexpected Data Collision (arbitration)
#define USI_TWI_NO_ACK_ON_DATA 0x05    // The slave did not acknowledge  all data
#define USI_TWI_NO_ACK_ON_ADDRESS 0x06 // The slave did not acknowledge  the address
#define USI_TWI_MISSING_START_CON 0x07 // Generated Start Condition not detected on bus
#define USI_TWI_MISSING_STOP_CON 0x08  // Generated Stop Condition not detected on bus

#define DELAY_T2TWI (_delay_us(T2_TWI))
#define DELAY_T4TWI (_delay_us(T4_TWI))

#define CTRL_REG_INIT   (0 << USISIE) | (0 << USIOIE) |  /* Disable Interrupts. */ \
	                    (1 << USIWM1) | (0 << USIWM0) |  /* Set USI in Two-wire mode. */ \
	                    (1 << USICS1) | (0 << USICS0)    /* Software stobe as counter clock source */
#define CTRL_REG_CLK_STROBE (1 << USICLK)
#define CTRL_REG_CLK_TOGGLE (1 << USITC)

#define STAT_REG_INIT   (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) | (1 << USIDC)
#define COUNTER_8BIT    (0x0 << USICNT0) /* Set 4-bit counter to 0 so that 2 x 8 clock edges occur */
#define COUNTER_1BIT    (0xE << USICNT0) /* Set 4-bit counter to 14 so that 2 x 1 clock edges occur */

void USI_TWI_Master_Initialise(void);
unsigned char USI_TWI_Start_Transceiver_With_Data(unsigned char *, unsigned char);
