#include <avr/io.h>
#include <avr/interrupt.h>


void eeprom_write(unsigned char ucAddress, unsigned char ucData)
{
    // Disable global interrupts
    cli();
    // Wait for completion of previous write
    while(EECR & (1<<EEPE))
        ;
    // Set Programming mode
    EECR = (0<<EEPM1)|(0<<EEPM0);
    // Set up address and data registers
    EEAR = ucAddress;
    EEDR = ucData;
    // Write logical one to EEMPE
    EECR |= (1<<EEMPE);
    // Start eeprom write by setting EEPE
    EECR |= (1<<EEPE);
    // Enable global interrupts
    sei();
}

unsigned char eeprom_read(unsigned char ucAddress)
{
    // Disable global interrupts
    cli();
    // Wait for completion of previous write
    while(EECR & (1<<EEPE))
        ;
    // Set up address register
    EEAR = ucAddress;
    // Start eeprom read by writing EERE
    EECR |= (1<<EERE);

    // Enable global interrupts
    sei();

    // Return data from data register
    return EEDR;
}
