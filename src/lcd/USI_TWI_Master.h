/*****************************************************************************
*
* Atmel Corporation
*
* File              : USI_TWI_Master.h
* Date              : $Date: 2016-7-15 $
* Updated by        : $Author: Atmel $
*
* Support mail      : avr@atmel.com
*
* Supported devices : All device with USI module can be used.
*                     The example is written for the ATmega169, ATtiny26 and ATtiny2313
*
* AppNote           : AVR310 - Using the USI module as a TWI Master
*
* Description       : This is an implementation of an TWI master using
*                     the USI module as basis. The implementation assumes the AVR to
*                     be the only TWI master in the system and can therefore not be
*                     used in a multi-master system.
* Usage             : Initialize the USI module by calling the USI_TWI_Master_Initialise()
*                     function. Hence messages/data are transceived on the bus using
*                     the USI_TWI_Start_Transceiver_With_Data() function. If the transceiver
*                     returns with a fail, then use USI_TWI_Get_Status_Info to evaluate the
*                     couse of the failure.
*
****************************************************************************/

#ifndef USI_TWI_MASTER_H
#define USI_TWI_MASTER_H

//********** Defines **********//
// Defines controlling timing limits
//#define TWI_FAST_MODE
#define F_CPU 8000000
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

// Defines controlling code generating
//#define PARAM_VERIFICATION
//#define NOISE_TESTING
#define SIGNAL_VERIFY

// USI_TWI messages and flags and bit masks
//#define SUCCESS   7
//#define MSG       0
/****************************************************************************
  Bit and byte definitions
****************************************************************************/
#define TWI_READ_BIT 0 // Bit position for R/W bit in "address byte".
#define TWI_ADR_BITS 1 // Bit position for LSB of the slave address bits in the init byte.
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

// ATtiny definitions
#  define DDR_USI DDRB
#  define PORT_USI PORTB
#  define PIN_USI PINB
#  define PORT_USI_SDA PORTB0
#  define PORT_USI_SCL PORTB2
#  define PIN_USI_SDA PINB0
#  define PIN_USI_SCL PINB2
#  define USI_START_COND_INT USISIF
#  define DDR_USI_CL DDR_USI
#  define PORT_USI_CL PORT_USI
#  define PIN_USI_CL PIN_USI

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
//********** Prototypes **********//

void USI_TWI_Master_Initialise();
unsigned char USI_TWI_Start_Transceiver_With_Data_Stop(unsigned char *, unsigned char, unsigned char);
unsigned char USI_TWI_Start_Transceiver_With_Data(unsigned char *, unsigned char);

#endif