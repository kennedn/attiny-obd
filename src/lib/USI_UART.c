#include "USI_UART.h"

#include <avr/interrupt.h>
#include <avr/io.h>

#define DATA_BITS 8
#define START_BIT 1
#define STOP_BIT 1
#define HALF_FRAME 5

#define USI_COUNTER_MAX_COUNT 16
#define USI_COUNTER_SEED_TRANSMIT (USI_COUNTER_MAX_COUNT - HALF_FRAME)
#define INTERRUPT_STARTUP_DELAY (0x1b / TIMER_PRESCALER)
#define TIMER0_SEED (256 - ((F_CPU / BAUDRATE) / TIMER_PRESCALER))

// The initial timer must overflow at either the 0.5 bit or 1.5 bit mark, this allows sampling to occur in the middle of each bit.
// A choice must be made based on the system clock and baud, Refer to Table 1. in the AVR307 reference notes
#if ((((F_CPU / BAUDRATE) / TIMER_PRESCALER) * 3 / 2) > (256 - INTERRUPT_STARTUP_DELAY))
#define INITIAL_TIMER0_SEED (256 - (((F_CPU / BAUDRATE) / TIMER_PRESCALER) * 1 / 2))
#define USI_COUNTER_SEED_RECEIVE (USI_COUNTER_MAX_COUNT - (START_BIT + DATA_BITS))
#else
#define INITIAL_TIMER0_SEED (256 - (((F_CPU / BAUDRATE) / TIMER_PRESCALER) * 3 / 2))
#define USI_COUNTER_SEED_RECEIVE (USI_COUNTER_MAX_COUNT - DATA_BITS)
#endif

#if TIMER_PRESCALER == 1
#define CLOCK_SOURCE_MASK (0 << CS02) | (0 << CS01) | (1 << CS00)
#elif TIMER_PRESCALER == 8
#define CLOCK_SOURCE_MASK (0 << CS02) | (1 << CS01) | (0 << CS00)
#error TIMER_PRESCALER set to 8 does not appear to work, perhaps due to timer resolution issues.
#else
#error TIMER_PRESCALER is not set to 1
#endif

#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1)
#if (UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK)
#error RX buffer size is not a power of 2
#endif

#define UART_TX_BUFFER_MASK (UART_TX_BUFFER_SIZE - 1)
#if (UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK)
#error TX buffer size is not a power of 2
#endif

/* General defines */
#define TRUE 1
#define FALSE 0

register unsigned char USI_UART_TxData asm("r15");  // Tells the compiler to store the byte to be transmitted in registry.

static unsigned char UART_RxBuf[UART_RX_BUFFER_SIZE];  // UART buffers. Size is definable in the header file.
static volatile unsigned char UART_RxHead;
static volatile unsigned char UART_RxTail;
static unsigned char UART_TxBuf[UART_TX_BUFFER_SIZE];
static volatile unsigned char UART_TxHead;
static volatile unsigned char UART_TxTail;

// Status byte holding flags.
static volatile union USI_UART_status {
    unsigned char status;
    struct
    {
        unsigned char ongoing_Transmission_From_Buffer : 1;
        unsigned char ongoing_Transmission_Of_Package : 1;
        unsigned char ongoing_Reception_Of_Package : 1;
        unsigned char reception_Buffer_Overflow : 1;
        unsigned char reception_Timeout : 1;
        unsigned char transmission_Timeout : 1;
        unsigned char flag6 : 1;
        unsigned char flag7 : 1;
    };
} USI_UART_status = {0};

unsigned char Bit_Reverse(unsigned char x) {
    unsigned char result = 0;
    for (int i = 0; i < 8; ++i) {
        result = (result << 1) | (x & 1);
        x >>= 1;
    }
    return result;
}


// Flush the UART buffers.
void USI_UART_Flush_Buffers(void) {
    UART_RxTail = 0;
    UART_RxHead = 0;
    UART_TxTail = 0;
    UART_TxHead = 0;
}

// Initialise USI for UART transmission.
void USI_UART_Initialise_Transmitter(void) {
    cli();
    TCNT0 = 0x00;
    TCCR0B = (1 << PSR0) | CLOCK_SOURCE_MASK;  // Reset the prescaler and start Timer0.
    TIFR = (1 << TOV0);                        // Clear Timer0 OVF interrupt flag.
    TIMSK |= (1 << TOIE0);                     // Enable Timer0 OVF interrupt.

    USICR = (0 << USISIE) | (1 << USIOIE) |                  // Enable USI Counter OVF interrupt.
            (0 << USIWM1) | (1 << USIWM0) |                  // Select Three Wire mode.
            (0 << USICS1) | (1 << USICS0) | (0 << USICLK) |  // Select Timer0 OVER as USI Clock source.
            (0 << USITC);

    USIDR = 0xFF;        // Make sure MSB is '1' before enabling USI_DO.
    USISR = 0xF0 |       // Clear all USI interrupt flags.
            0x0F;        // Preload the USI counter to generate interrupt at first USI clock.
    DDRB |= (1 << PB1);  // Configure USI_DO as output.

    USI_UART_status.ongoing_Transmission_From_Buffer = TRUE;

    sei();
}

// Initialise USI for UART reception.
// Note that this function only enables pinchange interrupt on the USI Data Input pin.
// The USI is configured to read data within the pinchange interrupt.
void USI_UART_Initialise_Receiver(void) {
    PORTB |= (1 << PB1) | (1 << PB0);                  // Enable pull up on USI DO and DI
    DDRB &= ~((1 << PB0) | (1 << PB1));                // Set USI DO and DI pins as input.
    TCCR0B = (0 << CS02) | (0 << CS01) | (0 << CS00);  // Stop Timer0.
    USICR = 0;                                         // Disable USI.
    GIFR = (1 << PCIF);                                // Clear pin change interrupt flag.
    GIMSK |= (1 << PCIE);                              // Enable pin change interrupt for PB5:PB0.
    PCMSK |= (1 << PCINT0);                            // Pin change mask register, enable for PB0
}

// Deactivate UART functions, e.g pin change interrupt and USI
void USI_UART_Deactivate(void) {
    TCCR0B = (0 << CS02) | (0 << CS01) | (0 << CS00);  // Stop Timer0.
    TIFR = (1 << TOV0);                                // Clear Timer0 OVF interrupt flag.
    TIMSK &= ~(1 << TOIE0);                            // Disable Timer0 OVF interrupt.
    USICR = 0;                                         // Disable USI.
    GIFR = (1 << PCIF);                                // Clear pin change interrupt flag.
    GIMSK &= ~(1 << PCIE);                             // Disable pin change interrupt for PB5:PB0.
    PCMSK &= ~(1 << PCINT0);                           // Clear PB0 pin change mask register
}

// Puts data in the transmission buffer, after reversing the bits in the byte.
// Initiates the transmission routines if not already started.
void USI_UART_Transmit_Byte(unsigned char data) {
    unsigned char tmphead;

    tmphead = (UART_TxHead + 1) & UART_TX_BUFFER_MASK;  // Calculate buffer index.
    while (tmphead == UART_TxTail)
        ;                                     // Wait for free space in transmit buffer (a transfer is on-going).
    UART_TxBuf[tmphead] = Bit_Reverse(data);  // Reverse the order of the bits in the data byte and store data in buffer.
    UART_TxHead = tmphead;                    // Store new index.

    if (!USI_UART_status.ongoing_Transmission_From_Buffer)  // Start transmission from buffer (if not already started).
    {
        while (USI_UART_status.ongoing_Reception_Of_Package)
            ;  // Wait for USI to finsh reading incoming data.
        USI_UART_Initialise_Transmitter();
    }
}

void USI_UART_Transmit_CString(char *string) {
    while (*string) {
        // Set USI DI as output, drops receieved data during the transmission window by making sure the pin change interrupt cannot fire
        DDRB |= 1 << PB0;
        USI_UART_Transmit_Byte(*string++);
    }
}
unsigned char USI_UART_Data_Transmitting(void) {
    return USI_UART_status.ongoing_Transmission_From_Buffer;
}

// Returns a byte from the receive buffer. Waits if buffer is empty.
unsigned char USI_UART_Receive_Byte(void) {
    unsigned char tmptail;

    if (UART_RxHead == UART_RxTail) {                   // No data in receive buffer
        return '\0';
    }

    tmptail = (UART_RxTail + 1) & UART_RX_BUFFER_MASK;  // Calculate buffer index
    UART_RxTail = tmptail;                              // Store new index
    return Bit_Reverse(UART_RxBuf[tmptail]);            // Reverse the order of the bits in the data byte before it returns data from the buffer.
}

// Copies `n` bytes from the UART receive circular buffer to a destination buffer as a null terminated char array.
// @param buff Pointer to a destination buffer.
// @param n Number of bytes to be extracted.
void USI_UART_Copy_Receive_Buffer(char *buff, unsigned char n) {
    do {
        unsigned char buff_index = (UART_RxHead + UART_RX_BUFFER_SIZE - n) & UART_RX_BUFFER_MASK;
        unsigned char c = Bit_Reverse(UART_RxBuf[buff_index]);
        if (c != '\r' && c != ' ') {
            *(buff++) = c;
        }
    } while (--n);
    *buff = '\0';  // Null terminate
}

// Check if there is data in the receive buffer.
unsigned char USI_UART_Data_In_Receive_Buffer(void) {
    return (UART_RxHead != UART_RxTail);  // Return 0 (FALSE) if the receive buffer is empty.
}

// The pin change interrupt is used to detect USI_UART reception.
// It is here the USI is configured to sample the UART signal.
ISR(SIG_PIN_CHANGE) {
    if (!( PINB & (1 << PB0) )) {                                   // If the USI DI pin is low, then it is likely the start of a byte
        TCNT0 = INTERRUPT_STARTUP_DELAY + INITIAL_TIMER0_SEED;  // Plant TIMER0 seed to match baudrate (incl interrupt start up time.).
        TCCR0B = (1 << PSR0) | CLOCK_SOURCE_MASK;               // Reset the prescaler and start Timer0.
        TIFR = (1 << TOV0);                                     // Clear Timer0 OVF interrupt flag.
        TIMSK |= (1 << TOIE0);                                  // Enable Timer0 OVF interrupt.

        USICR = (0 << USISIE) | (1 << USIOIE) |                  // Enable USI Counter OVF interrupt.
                (0 << USIWM1) | (1 << USIWM0) |                  // Select Three Wire mode.
                (0 << USICS1) | (1 << USICS0) | (0 << USICLK) |  // Select Timer0 OVER as USI Clock source.
                (0 << USITC);
        // Note that enabling the USI will also disable the pin change interrupt.
        USISR = 0xF0 |                     // Clear all USI interrupt flags.
                USI_COUNTER_SEED_RECEIVE;  // Preload the USI counter to generate interrupt.

        GIMSK &= ~(1 << PCIE);  // Disable pin change interrupt for PB5:PB0.

        USI_UART_status.ongoing_Reception_Of_Package = TRUE;
    }
}

// The USI Counter Overflow interrupt is used for moving data between memory and the USI data register.
// The interrupt is used for both transmission and reception.
ISR(SIG_USI_OVERFLOW) {
    unsigned char tmphead, tmptail;

    // Check if we are running in Transmit mode.
    if (USI_UART_status.ongoing_Transmission_From_Buffer) {
        // If ongoing transmission, then send second half of transmit data.
        if (USI_UART_status.ongoing_Transmission_Of_Package) {
            USI_UART_status.ongoing_Transmission_Of_Package = FALSE;  // Clear on-going package transmission flag.

            USISR = 0xF0 | (USI_COUNTER_SEED_TRANSMIT);  // Load USI Counter seed and clear all USI flags.
            USIDR = (USI_UART_TxData << 3) | 0x07;       // Reload the USIDR with the rest of the data and a stop-bit.
        }
        // Else start sending more data or leave transmit mode.
        else {
            // If there is data in the transmit buffer, then send first half of data.
            if (UART_TxHead != UART_TxTail) {
                USI_UART_status.ongoing_Transmission_Of_Package = TRUE;  // Set on-going package transmission flag.

                tmptail = (UART_TxTail + 1) & UART_TX_BUFFER_MASK;  // Calculate buffer index.
                UART_TxTail = tmptail;                              // Store new index.
                USI_UART_TxData = UART_TxBuf[tmptail];              // Read out the data that is to be sent. Note that the data must be bit reversed before sent.
                                                                    // The bit reversing is moved to the application section to save time within the interrupt.
                USISR = 0xF0 | (USI_COUNTER_SEED_TRANSMIT);         // Load USI Counter seed and clear all USI flags.
                USIDR = (USI_UART_TxData >> 2) | 0x80;              // Copy (initial high state,) start-bit and 6 LSB of original data (6 MSB
                                                                    //  of bit of bit reversed data).
            }
            // Else enter receive mode.
            else {
                USI_UART_status.ongoing_Transmission_From_Buffer = FALSE;
                USI_UART_Initialise_Receiver();
            }
        }
    }

    // Else running in receive mode.
    else {
        USI_UART_status.ongoing_Reception_Of_Package = FALSE;

        tmphead = (UART_RxHead + 1) & UART_RX_BUFFER_MASK;  // Calculate buffer index.

        if (tmphead == UART_RxTail)  // If buffer is full trash data and set buffer full flag.
        {
            USI_UART_status.reception_Buffer_Overflow = TRUE;  // Store status to take actions elsewhere in the application code
        } else                                                 // If there is space in the buffer then store the data.
        {
            UART_RxHead = tmphead;        // Store new index.
            UART_RxBuf[tmphead] = USIDR;  // Store received data in buffer. Note that the data must be bit reversed before used.
        }                                 // The bit reversing is moved to the application section to save time within the interrupt.
        USI_UART_Initialise_Receiver();
    }
}

// Timer0 Overflow interrupt is used to trigger the sampling of signals on the USI ports.
ISR(SIG_OVERFLOW0) {
    TCNT0 += TIMER0_SEED;  // Reload the timer,
                           // current count is added for timing correction.
}
