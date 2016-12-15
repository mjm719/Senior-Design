//******************************************************************************
//	--Mike McConville--
//
//	This file is the to be used for a Master MSP430 talking to a slave MSP430
//	via I2C in both write and read modes. This file is the master portion of
//	the code.
//
//                                /|\  /|\
//               MSP430G2xx3      10k  10k     MSP430G2xx3
//                   slave         |    |        master
//             -----------------   |    |  -----------------
//           -|XIN  P1.7/UCB0SDA|<-|---+->|P1.7/UCB0SDA  XIN|-
//            |                 |  |      |                 |
//           -|XOUT             |  |      |             XOUT|-
//            |     P1.6/UCB0SCL|<-+----->|P1.6/UCB0SCL     |
//            |                 |         |                 |
//
//*****************************************************************************

#include <msp430.h>

#define FALSE 0
#define TRUE 1

//***** Global Variables *****//
unsigned char TXData = 0x00;			// Data sending out
unsigned char TXByteCtr;				// Count of Data Bytes to be sent
unsigned char RorW;						// Determines whether the master is writing or reading
unsigned char RXData;					// Data coming in
long num_to_delay = 50000;				// Sets the number of clk cycles to wait in a while, therefore setting the total time to wait
unsigned int delay_count = 0;			// Count for how many times code has waited in start or stop. Goes up to num_to_delay
unsigned int OperationComplete = FALSE;	// Determines if you have completed your I2C transaction
unsigned int wait_operation_count = 0;	// Count for how many times code has waited in a while for operation completion
unsigned char read_data;				// Determines if you should continue and read data from slave



//***** Function Prototypes *****//
void Setup_TX(void);
void Setup_RX(void);
void wait_for_stop(void);
void wait_for_start(void);
void wait_for_operation_completion(void);


//***** Main Function *****//
int main(void){
 	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
	BCSCTL1 = CALBC1_8MHZ;					  // Set DCO CLK to 8MHz, this sets MCLK and SMCLK to 8MHz as well
	DCOCTL = CALDCO_8MHZ;					  // Finishes the step above

	P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	TXData = 0x00;                            // Holds TX data
	UCB0I2CIE = UCNACKIE;					  // Enable NACK interupts

	
	//***** While Loop *****//
	while (1){
		//Write Section
		Setup_TX();								// Call Setup Transfer function
		TXByteCtr = 1;    						// Load TX byte counter
		wait_for_stop();						// Call wait for stop function
		UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
		wait_for_operation_completion();		// Call wait for operation completion function

		//Read Section
		Setup_RX();								// Call Setup Receive function
		wait_for_stop();						// Call wait for stop function
	    UCB0CTL1 |= UCTXSTT;                    // I2C start condition
	    wait_for_start();						// Call wait for start function
	    if (read_data == 1){					// See if the conditions have been meet to receive data
		    UCB0CTL1 |= UCTXSTP;                // I2C stop condition
		    wait_for_operation_completion();	// Call wait for stop function
	    }
	    else {									// If the conditions for a RX have not been met, cancel the transaction
	    	UCB0CTL1 |= UCTXNACK;				// Send a NACK to the slave to clear the line
	    	UCB0CTL1 |= UCSWRST;                // Enable SW reset to clear the line
	    }

		TXData++;								// Increment TXData
	}
}

//------------------------------------------------------------------------------
// The USCIAB0TX_ISR is structured such that it can be used to transmit any
// number of bytes by pre-loading TXByteCtr with the byte count.
//------------------------------------------------------------------------------
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCIAB0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	// Writing to slave MSP430
	if (RorW == 0){
		if (TXByteCtr){                        		// Check TX byte counter
			UCB0TXBUF = TXData;                     // Load TX buffer
			IFG2 |= UCA0TXIFG;						// Clear USCI_B0 TX int flag
			TXByteCtr--;                            // Decrement TX byte counter
		}
		else{
			UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
			IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag
			OperationComplete = TRUE;				// Set the operation status to complete
		}
	}

	// Reading from slave MSP430
	else if (RorW == 1){
		RXData = UCB0RXBUF;                       	// Get RX data and store in RXData
		IFG2 &= ~UCA0RXIFG;							// Clear USCI_B0 RX int flag
		OperationComplete = TRUE;					// Set the operation status to complete
	}
}

// USCI_B0 State ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCIAB0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	if (UCB0STAT & UCNACKIFG){						// If you receive a NACK interrupt flag
		UCB0CTL1 |= UCTXSTP;						// Send a Stop
		UCB0STAT &= ~UCNACKIFG;						// Clear the NACK flag
		OperationComplete = TRUE;					// Set the operation status to complete
	}
}

//***** Setup Function *****//

// Function sets the master unit up to transmit data to the slave unit
void Setup_TX(void) {
	__disable_interrupt();					  // Do not allow interrupts while in this function
	RorW = 0;								  // Set transaction to write
	IE2 &= ~UCB0RXIE;						  // No longer allow RX interrupts
	IFG2 &= ~UCA0TXIFG;						  // Clear the TX IFG
	wait_for_stop();						  // Calls wait for stop
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 80;                             // fSCL = SMCLK/80 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x48;                         // Slave Address is 048h
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	IE2 |= UCB0TXIE;                          // Enable TX interrupt
}

// Function sets the master unit up to receive data from the slave unit
void Setup_RX(void){
	__disable_interrupt();					  // Do not allow interrupts while in this function
	RorW = 1;								  // Set transaction to read
	IE2 &= ~UCB0TXIE;						  // No longer allow TX interrupts
	IFG2 &= ~UCA0RXIFG;						  // Clear the RX IFG
	wait_for_stop();						  // Calls wait for stop
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 80;                             // fSCL = SMCLK/80 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x048;                        // Slave Address is 048h
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	IE2 |= UCB0RXIE;                          // Enable RX interrupt
}


//***** Sub-Routine Function *****//

// This function sits in a while loop until a stop condition has been received.
// If the stop has not been received after about approximately 37.5ms, clear the lines
void wait_for_stop(void){
	delay_count = 0;							// Reset your delay count
	while (UCB0CTL1 & UCTXSTP){             	// Check if a stop condition has been received
    	if (delay_count < num_to_delay){		// Make sure the actual count is less than the max count
    		delay_count++;						// Increment count
    		__delay_cycles(4);					// Delay another 4 clock cycles in an attempt to wait. Kept short so the function keeps
    											// rechecking for a stop condition
    	}
    	else{									// If you go past your count limit
    		UCB0CTL1 |= UCTXNACK;				// Send a NACK to the slave to clear the line
	    	UCB0CTL1 |= UCSWRST;                // Enable SW reset to clear the line
    		break;								// Forces an exit from the while loop
    	}
	}
}

// This function sits in a while loop until a start condition has been received.
// If the start has not been received after about approximately 37.5ms, clear the lines
void wait_for_start(void){
    read_data = 1;								// Set read_data to 1. If nothing goes wrong, the send_data will stay 1
    if (UCB0CTL1 & UCTXSTT){					// Check to see if a start condition has been received
    	delay_count = 0;
		while (UCB0CTL1 & UCTXSTT){             // Start condition sent?
	    	if (delay_count < num_to_delay){	// Make sure the actual count is less than the max count
	    		delay_count++;					// Increment count
	    	}
	    	else {								// If you go past your count limit
	    		read_data = 0;					// Set read data to 0 to indicate a problem occured
	    		UCB0CTL1 |= UCTXNACK;			// Send a NACK to the slave to clear the line
	    		UCB0CTL1 |= UCSWRST;            // Enable SW reset to clear the line
	    		break;							// Forces an exit from the while loop
	    	}
		}
    }
}

// This function enables I2C interrupts and checks to see if the I2C transaction ever finished. If it does, keep executing code.
// If not, reset the lines
void wait_for_operation_completion(void){
	__bis_SR_register(GIE);        					// Enable interrupts
	wait_operation_count = 0;						// Reset wait count
	while (OperationComplete == FALSE){				// Check the status of Operation Complete. If completed even in loop, variable will be set to TRUE
		wait_operation_count++;						// Increment wait count
		if (wait_operation_count > num_to_delay){	// Make sure the actual count is less than the max count
			UCB0CTL1 |= UCTXNACK;					// Send a NACK to the slave to clear the line
			UCB0CTL1 |= UCSWRST;                    // Enable SW reset to clear the line
			break;									// Forces an exit from the while loop
		}
	}
	OperationComplete = FALSE;						// Resets the Operation Complete variable after it has been used
}
