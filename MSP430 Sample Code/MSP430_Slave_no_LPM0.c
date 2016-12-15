//*******************************************************************************
//	--Mike McConville--
//
//	This file is the to be used for a Master MSP430 talking to a slave MSP430
//	via I2C in both write and read modes. This file is the slave portion of
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

volatile unsigned char RXData;				  // Used to hold the data the slave receives from the master's write
unsigned char TXData = 0x00;				  // Data to be sent back to the master when the master reads from the slave

void SetupI2C(void);

int main(void)
{
	//Input Pin Setup
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
	P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0, P1.6 is clock line, P1.7 is data line
	P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	SetupI2C();								  // Call Setup I1C
	while (1){
		__bis_SR_register(GIE);        		  // Enable global interrupts
	}
}

// USCI_B0 Data ISR
// Interrupt function used to handle TX and RX interrupts and carries out actions based off which one it is
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCIAB0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	if (IFG2 & UCB0RXIFG){                  	  // Handles Receive, determines if the reason for entering the interrupt function was a read interrupt, if so
		RXData = UCB0RXBUF;                       // Get RX data
		TXData = RXData;						  // Set the data you eventually wish to send back to the data you just received
		IFG2 &= ~UCA0RXIFG;						  // Clear the RX IFG
	}
	else {										  // Handles Transfer
		UCB0TXBUF = TXData++;                     // send TXData to the master
		IFG2 &= ~UCA0TXIFG;						  // Clear the TX IFG
	}
}

// USCI_B0 State ISR
// Interrupt service routine used to handle stop and start interrupts/ flags that have been thrown
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCIAB0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	UCB0STAT &= ~UCSTTIFG;                    // Clear start condition int flag
}

// Function is used to set up the device as a slave on I2C communication
void SetupI2C(){
	UCB0CTL0 = UCMODE_3 + UCSYNC;             // I2C Slave, synchronous mode
	UCB0I2COA = 0x48;                         // Own Address is 048h
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	UCB0I2CIE |= UCSTTIE;                     // Enable STT interrupt
	IE2 |= UCB0TXIE + UCB0RXIE;               // Enables both TX and RX interrupts to occur
}
