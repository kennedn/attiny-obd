#include "USI_TWI_Master.h"

#include <avr/io.h>

unsigned char USI_TWI_Master_Transfer(unsigned char);
unsigned char USI_TWI_Master_Stop(void);

struct USI_TWI_state {
    unsigned char addressMode : 1;
    unsigned char masterWriteDataMode : 1;
    unsigned char unused : 6;
} USI_TWI_state;

// Initalise pins and USI peripheral
void USI_TWI_Master_Initialise(void) {
    PORTB |= _BV(PB0) | _BV(PB2);  // Enable pullup on SCL and SDA to set high as released state
    DDRB |= _BV(PB0) | _BV(PB2);   // Enable SCL and SDA as output

    USIDR = 0xFF;  // Preload dataregister with "released level" data.
    USICR = CTRL_REG_INIT;
    USISR = STAT_REG_INIT;
}

// USI transmit and receive
// @param msg Data to send on the bus, first 8 bits must be 7 x address + r/w
// @param msgSize Size of `msg` in bytes
// @return Success / Error code
unsigned char USI_TWI_Start_Transceiver_With_Data(unsigned char *msg, unsigned char msgSize) {
    USI_TWI_state.addressMode = 1;

    if (!(*msg & (1 << TWI_READ_BIT)))  // The LSB in the address byte determines if is a masterRead or masterWrite operation.
    {
        USI_TWI_state.masterWriteDataMode = 1;
    }

    //  Release SCL to ensure that (repeated) Start can be performed
    PORTB |= _BV(PB2);  // Release SCL.
    while (!(PINB & _BV(PB2)))
        ;  // Wait for SCL to go high.
#ifdef TWI_FAST_MODE
    DELAY_T4TWI;  // Delay for T4TWI if TWI_FAST_MODE
#else
    DELAY_T2TWI;  // Delay for T2TWI if TWI_STANDARD_MODE
#endif

    // Generate Start Condition
    PORTB &= ~(_BV(PB0));  // Force SDA LOW.
    DELAY_T4TWI;
    PORTB &= ~(_BV(PB2));  // Pull SCL LOW.
    PORTB |= _BV(PB0);     // Release SDA.

    // Write address and Read/Write data
    do {
        //  If masterWrite cycle (or inital address tranmission)
        if (USI_TWI_state.addressMode || USI_TWI_state.masterWriteDataMode) {
            //  Write a byte
            PORTB &= ~(_BV(PB2));                   // Pull SCL LOW.
            USIDR = *(msg++);                       // Setup data.
            USI_TWI_Master_Transfer(COUNTER_8BIT);  // Send 8 bits on bus.

            //  Clock and verify (N)ACK from slave
            DDRB &= ~(_BV(PB0));  // Enable SDA as input.
            if (USI_TWI_Master_Transfer(COUNTER_1BIT) & (1 << TWI_NACK_BIT)) {
                return 0;
            }
            USI_TWI_state.addressMode = 0;  // Only perform address transmission once.
        }
        //  Else masterRead cycle
        else {
            //  Read a data byte
            DDRB &= ~(_BV(PB0));  // Enable SDA as input.
            *(msg++) = USI_TWI_Master_Transfer(COUNTER_8BIT);

            //  Prepare to generate ACK (or NACK in case of End Of Transmission)
            if (msgSize == 1)  // If transmission of last byte was performed.
            {
                USIDR = 0xFF;  // Load NACK to confirm End Of Transmission.
            } else {
                USIDR = 0x00;  // Load ACK. Set data register bit 7 (output for SDA) low.
            }
            USI_TWI_Master_Transfer(COUNTER_1BIT);  // Generate ACK/NACK.
        }
    } while (--msgSize);  // Until all data sent/received.

    USI_TWI_Master_Stop();  // Send a STOP condition on the TWI bus.

    //  Transmission successfully completed
    return 1;
}

// Shift data out and read responses on the TWI bus, data to be shifted out
// must be placed in the USIDR register before call
// @return Response data
unsigned char USI_TWI_Master_Transfer(unsigned char counter_value) {
    USISR = STAT_REG_INIT | counter_value;

    do {
        DELAY_T2TWI;
        USICR = USICR | CTRL_REG_CLK_STROBE | CTRL_REG_CLK_TOGGLE;  // Generate positive SCL edge.
        while (!(PINB & _BV(PB2)))
            ;  // Wait for SCL to go high.
        DELAY_T4TWI;
        USICR = USICR | CTRL_REG_CLK_STROBE | CTRL_REG_CLK_TOGGLE;  // Generate negative SCL edge.
    } while (!(USISR & (1 << USIOIF)));                             // Check for transfer complete.

    DELAY_T2TWI;

    unsigned char temp = USIDR;  // Read out data.
    USIDR = 0xFF;                // Release SDA.
    DDRB |= (_BV(PB0));          // Enable SDA as output.

    return temp;  // Return the data from the USIDR
}

// Generate TWI Stop Condition
unsigned char USI_TWI_Master_Stop(void) {
    PORTB &= ~(_BV(PB0));  // Pull SDA low.
    PORTB |= (_BV(PB2));   // Release SCL.
    while (!(PINB & _BV(PB2)))
        ;  // Wait for SCL to go high.
    DELAY_T4TWI;
    PORTB |= (_BV(PB0));  // Release SDA.
    DELAY_T2TWI;

    return 1;
}
