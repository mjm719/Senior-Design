#include <msp430.h>

#define windowSize 1

// Global Variables used in I2C
unsigned char *PCurrPress;                    			// Pointer to RX data
unsigned char RXByteCtr;
volatile unsigned char CurrentPressureBytes[2];       	// Allocate 2 bytes of RAM

unsigned int getPressure(void);
void SetupPressureRX(void);

int main(void){
	// Handle Watchdog timer along with setting up the clock to 1MHz
	WDTCTL = WDTPW + WDTHOLD;                 		// Stop WDT
	if (CALBC1_1MHZ==0xFF){							// If calibration constant erased
		while(1);                               	// do not load, trap CPU!!
	}
	DCOCTL = 0;                               		// Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;                    		// Set DCO
	DCOCTL = CALDCO_1MHZ;

	// Ensure P1 pins are initialized to 0
	P1OUT &= ~BIT0;                           		// P1.0 = 0
	P1DIR |= BIT0;                            		// P1.0 output

	// Setup I2C pins
	P1SEL |= BIT6 + BIT7;                     		// Assign I2C pins to USCI_B0
	P1SEL2|= BIT6 + BIT7;                     		// Assign I2C pins to USCI_B0

	// Setup P2.0 as output for ambient valve and set low
	P2SEL &= (~BIT0);
	P2DIR |= BIT0;
	P2OUT &= ~BIT0;

	// Setup P2.1 as output for pressure valve and set low
	P2SEL &= (~BIT1);
	P2DIR |= BIT1;
	P2OUT &= ~BIT1;

	// Setup P2.2 as output for boost converter and set low
	P2SEL &= (~BIT2);
	P2DIR |= BIT2;
	P2OUT &= ~BIT2;

	// Variable declarations
	unsigned int targetPSI;
	unsigned int currentPSI;
	unsigned int windowLow;
	unsigned int windowHigh;

	// For testing purposes only
	targetPSI = 42;

	while(1){
		// Ensure Valves are closed and get current pressure
		P2OUT &= ~BIT0;									// Ensure Ambient Valve is closed
		P2OUT &= ~BIT1;									// Ensure Pressure Valve is closed
		P2OUT &= ~BIT2;									// Ensure Boost Converter is not on
		//currentPSI = getPressure();						// Obtain Current pressure

		// Set variables for the window for the target range
		//windowLow = targetPSI - windowSize;
		//windowHigh = targetPSI + windowSize;
/*
		// Send the initial burst of air until the target pressure range has been met
		while(currentPSI < windowLow || currentPSI > windowHigh){
			// If current pressure is lower than the target range
			if(currentPSI < windowLow){
				P2OUT &= ~BIT0;							// Ensure Ambient Valve is closed
				P2OUT |= BIT2;							// Turn on the boost converter
				P2OUT |= BIT1;							// Open Pressure Valve
			}
			// If current pressure is higher than the target range
			else if(currentPSI > windowHigh){
				P2OUT &= ~BIT1;							// Ensure Pressure Valve is closed
				P2OUT |= BIT2;							// Turn on the boost converter
				P2OUT |= BIT0;							// Open Ambient Valve
			}
			// If current pressure is in the desired range
			else{
				P2OUT &= ~BIT0;							// Ensure Ambient Valve is closed
				P2OUT &= ~BIT1;							// Ensure Pressure Valve is closed
				__delay_cycles(1000000);				// Wait for the pressure to normalize for 1s
			}
			currentPSI = getPressure();					// Obtain Current Pressure before looping back through
		}
		*/
		P2OUT |= BIT2;							// Ensure Boost is open
		P2OUT |= BIT0;							// Ensure Ambient Valve is open
		P2OUT &= ~BIT1;							// Ensure Pressure Valve is closed
		__delay_cycles(100000);					// Wait for the pressure to normalize for 1s
		P2OUT &= ~BIT2;
		P2OUT &= ~BIT0;							// Ensure Ambient Valve is closed
		//P2OUT |= BIT1;							// Ensure Pressure Valve is open
		//currentPSI = getPressure();						// Obtain Current pressure
		__no_operation();
		__delay_cycles(2000000);				// Wait for the pressure to normalize for 1s

		// Congratulations!!! You have the correct tire pressure
	}
}


//**************INTERUPT SERVICE ROUTINES**************************************//

//-------------------------------------------------------------------------------
// The USCI_B0 data ISR is used to move received data from the I2C slave
// to the MSP430 memory. It is structured such that it can be used to receive
// any 2+ number of bytes by pre-loading RXByteCtr with the byte count.
//-------------------------------------------------------------------------------
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCIAB0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	RXByteCtr--;                              		// Decrement RX byte counter
	if (RXByteCtr){
		*PCurrPress++ = UCB0RXBUF;              	// Move RX data to address PCurrPress
	if (RXByteCtr == 1)                     		// Only one byte left?
		UCB0CTL1 |= UCTXSTP;                  		// Generate I2C stop condition
	}
	else{
		*PCurrPress = UCB0RXBUF;                	// Move final RX data to PCurrPress
		__bic_SR_register_on_exit(CPUOFF);      	// Exit LPM0
	}
}


//*****************FUNCTIONS****************************************************//

// Obtain the current pressure from the pressure sensor and store these 2 bytes into 1 16 bit long variable to be returned
unsigned int getPressure(void){
	// Get the current pressure
	SetupPressureRX;
    PCurrPress = (unsigned char *)CurrentPressureBytes;    	// Start of RX buffer
    RXByteCtr = 2;                          				// Load RX byte counter
    while (UCB0CTL1 & UCTXSTP);            					// Ensure stop condition got sent
    UCB0CTL1 |= UCTXSTT;                    				// I2C start condition
    __bis_SR_register(CPUOFF + GIE);        				// Enter LPM0 w/ interrupts. Remain in LPM0 until all data is RX'd

    // Store the current pressure into 1 16 bit long variable to be used by the main function
    unsigned int PressureCombined;
    PressureCombined = (CurrentPressureBytes[1] << 8) | CurrentPressureBytes[0];
    return PressureCombined;
}

// Prepares the microcontroller's registers for I2C RX from the pressure sensor
void SetupPressureRX(void){
	UCB0CTL1 |= UCSWRST;                      		// Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     		// I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            		// Use SMCLK, keep SW reset
	UCB0BR0 = 12;                             		// fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x28;                         		// Slave Address is 028h
	UCB0CTL1 &= ~UCSWRST;                     		// Clear SW reset, resume operation
	IE2 |= UCB0RXIE;                          		// Enable RX interrupt
}
