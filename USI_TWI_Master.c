/*****************************************************************************
*
* Atmel Corporation
*
* File              : USI_TWI_Master.c
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
*                     the USI_TWI_Transceive() function. The transceive function
*                     returns a status byte, which can be used to evaluate the
*                     success of the transmission.
*
****************************************************************************/
#include <avr/io.h>
#include "USI_TWI_Master.h"

unsigned char USI_TWI_Master_Transfer(unsigned char);
unsigned char USI_TWI_Master_Stop(void);

struct USI_TWI_state {
	unsigned char addressMode : 1;
	unsigned char masterWriteDataMode : 1;
	unsigned char unused : 6;
} USI_TWI_state;

/*---------------------------------------------------------------
 USI TWI single master initialization function
---------------------------------------------------------------*/
void USI_TWI_Master_Initialise()
{
	PORT_USI |= (1 << PIN_USI_SDA); // Enable pullup on SDA, to set high as released state.
	PORT_USI_CL |= (1 << PIN_USI_SCL); // Enable pullup on SCL, to set high as released state.

	DDR_USI_CL |= (1 << PIN_USI_SCL); // Enable SCL as output.
	DDR_USI |= (1 << PIN_USI_SDA); // Enable SDA as output.

	USIDR = 0xFF; // Preload dataregister with "released level" data.
	USICR = CTRL_REG_INIT;
	USISR = STAT_REG_INIT;
}

/*---------------------------------------------------------------
 USI Transmit and receive function. LSB of first byte in data
 indicates if a read or write cycles is performed. If set a read
 operation is performed.

 Function generates (Repeated) Start Condition, sends address and
 R/W, Reads/Writes Data, and verifies/sends ACK.

 Success or error code is returned. Error codes are defined in
 USI_TWI_Master.h
---------------------------------------------------------------*/
unsigned char USI_TWI_Start_Transceiver_With_Data(unsigned char *msg, unsigned char msgSize) {
	return USI_TWI_Start_Transceiver_With_Data_Stop(msg, msgSize, 1);
}

/*---------------------------------------------------------------
 USI Transmit and receive function.

 Same as USI_TWI_Start_Transceiver_With_Data() but with an additional
 parameter that defines if a Stop Condition should be send at the end
 of the transmission.
---------------------------------------------------------------*/
unsigned char USI_TWI_Start_Transceiver_With_Data_Stop(unsigned char *msg, unsigned char msgSize, unsigned char stop)
{
	USI_TWI_state.addressMode = 1;

	if (!(*msg & (1 << TWI_READ_BIT))) // The LSB in the address byte determines if is a masterRead or masterWrite operation.
	{
		USI_TWI_state.masterWriteDataMode = 1;
	}

	/* Release SCL to ensure that (repeated) Start can be performed */
	PORT_USI_CL |= (1 << PIN_USI_SCL); // Release SCL.
	while (!(PIN_USI_CL & (1 << PIN_USI_SCL))); // Verify that SCL becomes high.
#ifdef TWI_FAST_MODE
	DELAY_T4TWI; // Delay for T4TWI if TWI_FAST_MODE
#else
	DELAY_T2TWI; // Delay for T2TWI if TWI_STANDARD_MODE
#endif

	/* Generate Start Condition */
	PORT_USI &= ~(1 << PIN_USI_SDA); // Force SDA LOW.
	DELAY_T4TWI;
	PORT_USI_CL &= ~(1 << PIN_USI_SCL); // Pull SCL LOW.
	PORT_USI |= (1 << PIN_USI_SDA);  // Release SDA.

	/*Write address and Read/Write data */
	do {
		/* If masterWrite cycle (or inital address tranmission)*/
		if (USI_TWI_state.addressMode || USI_TWI_state.masterWriteDataMode) {
			/* Write a byte */
			PORT_USI_CL &= ~(1 << PIN_USI_SCL);       // Pull SCL LOW.
			USIDR = *(msg++);                         // Setup data.
			USI_TWI_Master_Transfer(COUNTER_8BIT);    // Send 8 bits on bus.

			/* Clock and verify (N)ACK from slave */
			DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
			if (USI_TWI_Master_Transfer(COUNTER_1BIT) & (1 << TWI_NACK_BIT)) {
				return 0;
			}
			USI_TWI_state.addressMode = 0; // Only perform address transmission once.
		}
		/* Else masterRead cycle*/
		else {
			/* Read a data byte */
			DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
			*(msg++) = USI_TWI_Master_Transfer(COUNTER_8BIT);

			/* Prepare to generate ACK (or NACK in case of End Of Transmission) */
			if (msgSize == 1) // If transmission of last byte was performed.
			{
				USIDR = 0xFF; // Load NACK to confirm End Of Transmission.
			} else {
				USIDR = 0x00; // Load ACK. Set data register bit 7 (output for SDA) low.
			}
			USI_TWI_Master_Transfer(COUNTER_1BIT); // Generate ACK/NACK.
		}
	} while (--msgSize); // Until all data sent/received.

	if (stop) {
		USI_TWI_Master_Stop(); // Send a STOP condition on the TWI bus.
	}

	/* Transmission successfully completed*/
	return 1;
}

/*---------------------------------------------------------------
 Core function for shifting data in and out from the USI.
 Data to be sent has to be placed into the USIDR prior to calling
 this function. Data read, will be return'ed from the function.
---------------------------------------------------------------*/
unsigned char USI_TWI_Master_Transfer(unsigned char counter_value)
{
	USISR = STAT_REG_INIT | counter_value;

	do {
		DELAY_T2TWI;
		USICR = USICR | CTRL_REG_CLK_STROBE | CTRL_REG_CLK_TOGGLE; // Generate positive SCL edge.
		while (!(PIN_USI_CL & (1 << PIN_USI_SCL)))
			; // Wait for SCL to go high.
		DELAY_T4TWI;
		USICR = USICR | CTRL_REG_CLK_STROBE | CTRL_REG_CLK_TOGGLE; // Generate negative SCL edge.
	} while (!(USISR & (1 << USIOIF))); // Check for transfer complete.

	DELAY_T2TWI;
	
	unsigned char temp  = USIDR;   // Read out data.
	USIDR = 0xFF;                  // Release SDA.
	DDR_USI |= (1 << PIN_USI_SDA); // Enable SDA as output.

	return temp; // Return the data from the USIDR
}

/*---------------------------------------------------------------
 Function for generating a TWI Stop Condition. Used to release
 the TWI bus.
---------------------------------------------------------------*/
unsigned char USI_TWI_Master_Stop(void)
{
	PORT_USI &= ~(1 << PIN_USI_SDA); // Pull SDA low.
	PORT_USI_CL |= (1 << PIN_USI_SCL);  // Release SCL.
	while (!(PIN_USI_CL & (1 << PIN_USI_SCL)))
		; // Wait for SCL to go high.
	DELAY_T4TWI;
	PORT_USI |= (1 << PIN_USI_SDA); // Release SDA.
	DELAY_T2TWI;

	return 1;
}
