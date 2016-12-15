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

unsigned char TXData;				//Variable to hold the data you want to write
unsigned char TXByteCtr;			//Variable to determine how many more bytes of data you have left to send
unsigned char RorW;					//Variable to tell the master in the the interupt whether to write or read
unsigned char RXData;				//Variable to store the data you reveive from a read

void Setup_TX(void);			//Function prototype for setting up a write
void Setup_RX(void);			//Function prototype for setting up a read

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;                 // Stop watch dog timer
	P1OUT &= ~BIT0;                           // P1.0 = 0, sets LED1 to 0, which turns the LED off
	P1DIR |= BIT0;                            // P1.0 output, enables use of LED1
	P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0, P1.6 is clock line, P1.7 is data line
	P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	TXData = 0x00;                            // First part of data I would eventually like to send

	while (1)		// code continues to go through this section forever, will never go back to the rest of the main function
	{
		//Write Section
		Setup_TX();				//Function call to setup a write for the master
		TXByteCtr = 1;                          //Set the number of data bytes to be sent to 1
		while (UCB0CTL1 & UCTXSTP);             //Check to see if the line is currently open
		UCB0CTL1 |= UCTR + UCTXSTT;             //UCTR = set to transmit, UCTXSTT sends the start bit reserving communication on the line between the master and slave
												//UCTXSTT also sends the address that is currently being stored in UCB0I2CSA, which starts the communication and reserves the line
		__bis_SR_register(CPUOFF + GIE);        // Enter low power mode and wait for an interrupt, interupts will still be serviced
												// Remain in LPM0 until all data
												// is TX'd

		//Read Section
		Setup_RX();				//Function call to setup the master reading from the slave
	    while (UCB0CTL1 & UCTXSTP);             //Check to see if the line is currently open
	    UCB0CTL1 |= UCTXSTT;                    //UCTXSTT sends the start bit reserving communication on the line between the master and slave
												//UCTXSTT also sends the address that is currently being stored in UCB0I2CSA, which starts the communication and reserves the line
	    while (UCB0CTL1 & UCTXSTT);             //Check to see if the master and slave have opened the I2C line for communication
	    UCB0CTL1 |= UCTXSTP;                    //Send a stop condition and wait for the slave that was addressed to respond with the data you want to read
	    __bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts

	    //Compare Section
	    if (RXData != TXData){               	//Check to make sure the data you wrote matches what you read. If not, go inside the if statement
			P1OUT |= BIT0;                        // P1.0 = 1, turn LED1 on so that you know the data did not match
			while (1);                            // Trap CPU. The code will stop here and LED1 will remain on.
	    }

		TXData++;                               // Increment data to be sent
	}
}

//------------------------------------------------------------------------------
// The USCIAB0TX_ISR is structured such that it can be used to transmit any
// number of bytes by pre-loading TXByteCtr with the byte count.
//------------------------------------------------------------------------------
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)			//Pre-defined code written by
#pragma vector = USCIAB0TX_VECTOR												//TI in order to handle
__interrupt void USCIAB0TX_ISR(void)											//interrupts.
#elif defined(__GNUC__)															//
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCIAB0TX_ISR (void)			//Interupt Service routine to be handled
#else																			//
#error Compiler not supported!													
#endif																			//
{
	if (RorW == 0){					//Determine if master is set to write or read, if write, then
		if (TXByteCtr){                        		//Check to see if you have more data bytes to send
			UCB0TXBUF = TXData;                     //********This sends the data you currently have stored in TXData to the device you are addressing*************
			TXByteCtr--;                            //Decrease the amount of data bytes you have left to send by one
		}
		else{						//If no more data bytes are left to be sent
			UCB0CTL1 |= UCTXSTP;                    //Stop bit, clears line for communication between other devices
			IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag, hard coded to make sure the flag that brought you into this interrupt is cleared
			__bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
		}
	}
	if (RorW == 1){					//Determine if master is set to write or read, if read, then
		RXData = UCB0RXBUF;                       //**********Store data you are reading from the slave into RXData
		__bic_SR_register_on_exit(CPUOFF);        // Exit LPM0
	}
}

//Function sets the master device up to receive TX interrupts and prepare for writing data
void Setup_TX(void) {
	__disable_interrupt();		//Clears interrupts that have been enabled
	RorW = 0;					//Set the master to write when in interrupts
	IE2 &= ~UCB0RXIE;			//Hard coded to ensure that RX interrupts are no longer enabled
	IFG2 &= ~UCA0TXIFG;			//Hard coded to ensure that the TX interrupt flag is cleared
	while (UCB0CTL1 & UCTXSTP);               //Make sure line is availible to communicate over
	UCB0CTL1 |= UCSWRST;                      //Enables the uses of switch reset flags
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     //I2C Master, synchronous mode, sets the device as master and allows data to flow both ways
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            ///Use SMCLK, keep SW reset, sets what type of clock to use
	UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz, sets clock period
	UCB0BR1 = 0;
	UCB0I2CSA = 0x48;                         // Slave Address is 048h, this is the slave device to be addressed
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation, makes sure the switch reset flag is 0
	IE2 |= UCB0TXIE;                          // Enable TX interrupt, this will allow you to write data
}

//Function sets the master device up to receive RX interrupts and prepare for reading data
void Setup_RX(void){
	__disable_interrupt();		//Clears interrupts that have been enabled
	RorW = 1;					//Sets the master to read when in interrupts
	IE2 &= ~UCB0TXIE;			//Hard coded to ensure that TX interrupts are no longer enabled
	IFG2 &= ~UCA0RXIFG;			//Hard coded to ensure that the RX interrupt flag is cleared
	while (UCB0CTL1 & UCTXSTP);               //Make sure the line is availible to communicate over
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x048;                        // Slave Address is 048h
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	IE2 |= UCB0RXIE;                          // Enable RX interrupt, this will allow you to read data
}
