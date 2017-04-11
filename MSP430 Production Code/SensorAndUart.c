/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430G2xx3 Demo - USCI_B0 I2C Master RX multiple bytes from MSP430 Slave
//
//  Description: This demo connects two MSP430's via the I2C bus. The slave
//  transmits to the master. This is the master code. It continuously
//  receives an array of data and demonstrates how to implement an I2C
//  master receiver receiving multiple bytes using the USCI_B0 TX interrupt.
//  ACLK = n/a, MCLK = SMCLK = BRCLK = default DCO = ~1.2MHz
//
//  *** to be used with "msp430g2xx3_uscib0_i2c_11.c" ***
//
//                                /|\  /|\
//               MSP430G2xx3      10k  10k     MSP430G2xx3
//                   slave         |    |        master
//             -----------------   |    |  -----------------
//           -|XIN  P3.1/UCB0SDA|<-|---+->|P3.1/UCB0SDA  XIN|-
//            |                 |  |      |                 |
//           -|XOUT             |  |      |             XOUT|-
//            |     P3.2/UCB0SCL|<-+----->|P3.2/UCB0SCL     |
//            |                 |         |                 |
//
//  D. Dang
//  Texas Instruments Inc.
//  February 2011
//  Built with CCS Version 4.2.0 and IAR Embedded Workbench Version: 5.10
//******************************************************************************
#include <msp430.h>
#include <stdio.h>

unsigned char *PRxData;                     // Pointer to RX data
unsigned char RXByteCtr;
volatile unsigned char RxBuffer[2];       	// Allocate 128 byte of RAM

unsigned int OutputMax = 0x3999;
unsigned int OutputMin = 0x0666;
unsigned char PMax = 150;
unsigned char PMin = 1;

unsigned int Pressure;
unsigned int Output;
unsigned int OutputTemp;

char string1[3] = {'a', 'B', 'c'};
char string2[2];
char i;
char j = 0;

int main(void){
	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

	P1DIR = 0xFF;                             // All P1.x outputs
	P1OUT = 0;                                // All P1.x reset
	P2DIR = 0xFF;                             // All P2.x outputs
	P2OUT = 0;                                // All P2.x reset
	P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2 ;                    // P1.1 = RXD, P1.2=TXD

	DCOCTL = 0;                               	//Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;                    	//Set DCO
	DCOCTL = CALDCO_1MHZ;

	P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 12;                             // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = 0x28;                         // Slave Address is 028h
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	IE2 |= UCB0RXIE;                          // Enable RX interrupt

	// Setup P2.0 as output for ambient valve and set low
	P2SEL &= (~BIT0);
	P2DIR |= BIT0;
	P2OUT &= ~BIT0;

	// Setup P2.1 as output for pressurized valve and set low
	P2SEL &= (~BIT1);
	P2DIR |= BIT1;
	P2OUT &= ~BIT1;

	// Setup P2.2 as output for boost converter and set low
	P2SEL &= (~BIT2);
	P2DIR |= BIT2;
	P2OUT &= ~BIT2;

	//Turn Off All IFG flags
	IFG2 &= ~UCA0TXIFG;

	int currentPressure;
/*
	UCA0CTL1 |= UCSSEL_2;                     	//SMCLK
	UCA0BR0 = 104;                            	//1MHz 9600
	UCA0BR1 = 0;                              	//1MHz 9600
	UCA0MCTL = UCBRS0;                        	//Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     	// **Initialize USCI state machine**
*/
	__delay_cycles(1000000);

	while (1){
		PRxData = (unsigned char *)RxBuffer;    // Start of RX buffer
		RXByteCtr = 2;                          // Load RX byte counter
		while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
		UCB0CTL1 |= UCTXSTT;                    // I2C start condition
		__bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
												// Remain in LPM0 until all data
												// is RX'd
		__no_operation();                       // Set breakpoint >>here<< and
												// read out the RxBuffer buffer
		//OutputTemp = RxBuffer[0] << 8 | RxBuffer[1];
		//for(x = 0; x < sizeof(OutputTemp) - 2; x++){
		//	Output[x] = OutputTemp[x];
		//}
		//Pressure = (((Output - OutputMin) * (PMax - PMin)) / (OutputMax - OutputMin)) + PMin;
/*
		if (RxBuffer[0] == 0x06){
			string1[0] = '6';
		}
		else{
			string1[0] = 'Z';
		}*/
		/*string1[0] = RxBuffer[0];
		string1[1] = RxBuffer[1];
		//string1[2] = '\n';

		i = 0;
		j = 0;
		IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
		__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled
		*/

		currentPressure = (RxBuffer[0] << 8) | RxBuffer[1];

/*		while(currentPressure < 4000){
			//if(currentPressure < 4000){
				//P2OUT &= ~BIT0;							// Ensure Ambient Valve is closed
				P2OUT |= BIT2;							// Turn on the boost converter
				P2OUT |= BIT1;							// Open Pressure Valve
			//}
			// If current pressure is higher than the target range
			//else if(currentPSI > 4500){
			//	P2OUT &= ~BIT1;							// Ensure Pressure Valve is closed
			//	P2OUT |= BIT2;							// Turn on the boost converter
			//	P2OUT |= BIT0;							// Open Ambient Valve
			//}

			__delay_cycles(500000);
		  //TACTL = TASSEL_2 + ID_3 + MC_2 + TAIE;           // SMCLK, contmode, interrupt
		  //__bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ interrupt
		  //TACTL = TACLR + MC_0;           // SMCLK, contmode, interrupt
			P2OUT &= ~BIT2;
			P2OUT &= ~BIT1;
			//P2OUT &= ~BIT0;
			__delay_cycles(1000000);
/*
			//Code that works for lowering pressure
			P2OUT |= BIT2;							// Turn on the boost converter
			P2OUT |= BIT0;							// Open Ambient Valve
			//}
			// If current pressure is in the desired range
			//__delay_cycles(500000);
			  TACTL = TASSEL_2 + ID_3 + MC_2 + TAIE;           // SMCLK, contmode, interrupt
			  __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0 w/ interrupt
			  TACTL = TACLR + MC_0;           // SMCLK, contmode, interrupt
			P2OUT &= ~ BIT2;
			P2OUT &= ~ BIT0;
			__delay_cycles(1000000);

			PRxData = (unsigned char *)RxBuffer;    // Start of RX buffer
			RXByteCtr = 2;                          // Load RX byte counter
			while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
			UCB0CTL1 |= UCTXSTT;                    // I2C start condition
			__bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
													// Remain in LPM0 until all data
													// is RX'd
			__no_operation();                       // Set breakpoint >>here<< and
													// read out the RxBuffer buffer

			currentPressure = (RxBuffer[0] << 8) | RxBuffer[1];

		}*/
		P2OUT &= ~BIT0;							// Ensure Ambient Valve is closed
		P2OUT &= ~BIT2;
		//__delay_cycles(1000000);				// Wait for the pressure to normalize for 1s
				/*
		__delay_cycles(1000000);
		P2OUT |= BIT2;							// Ensure Boost is open
		P2OUT |= BIT0;							// Ensure Ambient Valve is open
		__delay_cycles(500000);					// Wait for the pressure to normalize for 1s
		P2OUT &= ~BIT2;
		P2OUT &= ~BIT0;	*/						// Ensure Ambient Valve is closed
		__delay_cycles(1000000);

	}
}

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
	if (IFG2 & UCB0RXIFG){
		RXByteCtr--;                              // Decrement RX byte counter
		if (RXByteCtr){
			*PRxData++ = UCB0RXBUF;                 // Move RX data to address PRxData
			if (RXByteCtr == 1)                     // Only one byte left?
				UCB0CTL1 |= UCTXSTP;                  // Generate I2C stop condition
			}
			else{
				*PRxData = UCB0RXBUF;                   // Move final RX data to PRxData
				//IE2 &= ~UCB0RXIE;                       // Enable RX interrupt
				__bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
		}
	}/*
	if (IFG2 & UCA0TXIFG){
		UCA0TXBUF = string1[i++];                 // TX next character
		if (i == sizeof string1){                  // TX over?
			IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
			//__bic_SR_register_on_exit(CPUOFF);      //Exit LPM0
			//WDTCTL = WDTPW + WDTTMSEL;                // Start WDT in interrupt mode
			//IE1 |= WDTIE;                       // Enable WDT interrupt
			IE2 |= UCA0RXIE;                        // Enable USCI_A0 RX interrupt
		}
	}
*/
}

// USCI A0/B0 Receive ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
  string2[j++] = UCA0RXBUF;
  if (j > sizeof string2 - 1)
  {
	IE2 &= ~UCA0RXIE;                       // Disable USCI_A0 RX interrupt
    __bic_SR_register_on_exit(CPUOFF);      //Exit LPM0
  }
}

// Timer_A3 Interrupt Vector (TA0IV) handler
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A1_VECTOR))) Timer_A (void)
#else
#error Compiler not supported!
#endif
{
 switch( TA0IV )
 {
   case  2: break;                          // CCR1 not used
   case  4: break;                          // CCR2 not used
   case 10:                   // overflow
   	   	    __bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
            break;
 }
}



