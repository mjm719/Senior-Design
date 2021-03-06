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
//   MSP430G2xx3 Demo - USCI_A0, Ultra-Low Pwr UART 9600 RX/TX, 32kHz ACLK
//
//   Description: This program demonstrates a full-duplex 9600-baud UART using
//   USCI_A0 and a 32kHz crystal.  The program will wait in LPM3, and receive
//   a UART_TX into RAM, and echo back the complete string.
//   ACLK = BRCLK = LFXT1 = 32768Hz, MCLK = SMCLK = DCO ~1.2MHz
//   Baud rate divider with 32768Hz XTAL @9600 = 32768Hz/9600 = 3.41
//* An external watch crystal is required on XIN XOUT for ACLK *//
//
//                MSP430G2xx3
//             -----------------
//         /|\|              XIN|-
//          | |                 | 32kHz
//          --|RST          XOUT|-
//            |                 |
//            |     P1.2/UCA0TXD|------------>
//            |                 | 9600 - 8N1
//            |     P1.1/UCA0RXD|<------------
//
//
//   D. Dang
//   Texas Instruments Inc.
//   February 2011
//   Built with CCS Version 4.2.0 and IAR Embedded Workbench Version: 5.10
//******************************************************************************
#include <msp430.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

unsigned char *PRxData;                     // Pointer to RX data
unsigned char RXByteCtr;
volatile unsigned char RxBuffer[2];       	// Allocate 128 byte of RAM

unsigned char StatusByte;			//Status Information to be sent back to the user's Android application

//StatusByte Layout
//Bit		7			6		5			4				3				2				1				0
//Flag		?????		????	????????	lowCO2			BatteryShutdown	CurrentStatus1	CurrentStatus0	SensorFailure
//Bit Val	128			64		32			16				8				4				2				1

//Possibly front/back flag, power on/ off flag

#define SensorFailure		1
#define CurrentStatus0 		2
#define CurrentStatus1 		4
#define BatteryShutdown 	8
#define lowCO2				16

//To Turn Bit On 	StatusByte |= Flag
//To Turn Bit Off	StatusByte &= ~Flag
//To Check if Bit is On		if (StatusByte & Flag)
//To Check if Bit is Off	if (StatusByte ^ Flag)
//Ex. To Turn SensorFailure On		StatusByte |= SensorFailure;
//If a flag is on, that indicates there was a failure with that flag

//newStatus Layout
//Bit		7			6		5			4				3			2			1			0
//Flag		?????		????	????????	???????			??????		??????		newCO2		ForceIdle
//Bit Val	128			64		32			16				8			4			2			1

// Make desiredPressure 0
unsigned int desiredPressure = 0;
unsigned char newStatus;

#define ForceIdle			1
#define newCO2				2

char UART_TX[4] = {0x00, 0x00, 0x00, 0x00};
char UART_RX[4];
char i;
char j = 0;
int currentPressure;
unsigned char msbPressure;

#define window 				150

unsigned int previousPressure;
unsigned char count;

void getPressure(void);
void sendUART(void);
void postPressure(void);
void getValuesUART(void);
void wait(unsigned int waitCount);

int main(void)
{

	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

	DCOCTL = 0;                               	//Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;                    	//Set DCO
	DCOCTL = CALDCO_1MHZ;

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

	P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
	P1SEL2 = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD

	P1DIR = 0x01;
	// intialize bit 0 of P1 to 0
	P1OUT = 0x00;

	//I2C Setup
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

	DCOCTL = 0;                               	//Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;                    	//Set DCO
	DCOCTL = CALDCO_1MHZ;

	UCA0CTL1 |= UCSSEL_2;                     	//SMCLK
	UCA0BR0 = 104;                            	//1MHz 9600
	UCA0BR1 = 0;                              	//1MHz 9600
	UCA0MCTL = UCBRS0;                        	//Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

	int count = 0;
	int baseCase = 1;
	unsigned int diff;

	while(1){									//While true to always execute code

		previousPressure = currentPressure;
		getPressure();
		currentPressure = (RxBuffer[0] << 8) | RxBuffer[1];

		StatusByte |= CurrentStatus0;
		StatusByte &= ~CurrentStatus1;

		postPressure();
		sendUART();
		getValuesUART();

		baseCase = 1;

		// && !(StatusByte & lowCO2) 			place this in the while

		while(((currentPressure < desiredPressure - window) || (currentPressure > desiredPressure + window)) && desiredPressure){
			baseCase = 0;
			if(currentPressure < desiredPressure - window){				//Pressure is lower than the desired pressure
				P2OUT &= ~BIT0;
				P2OUT |= BIT2;
				P2OUT |= BIT1;
				StatusByte |= CurrentStatus0;
				StatusByte |= CurrentStatus1;
				if((currentPressure < previousPressure + 15) && count){				//Check for if the CO2 cannister seems to be empty
					P2OUT &= ~BIT2;
					P2OUT &= ~BIT1;
					P2OUT &= ~BIT0;
					StatusByte |= lowCO2;
					break;
				}
				else{
					StatusByte &= ~lowCO2;
				}
				count++;
			}
			else if(currentPressure > desiredPressure + window){					//Pressure is higher than the desired pressure
				P2OUT &= ~ BIT1;
				P2OUT |= BIT2;							// Turn on the boost convert2er
				P2OUT |= BIT0;							// Open Ambient Valve
				StatusByte &= ~CurrentStatus0;
				StatusByte |= CurrentStatus1;
			}

			// Leave the valve on for 0.52 seconds
			diff = desiredPressure - currentPressure;
			diff = abs(diff);

			if(diff > 1000){				//Large Pressure Change
				wait(3);					//Wait for 3 periods 1.56 seconds
			}
			else if(diff > 500){			//Medium Pressure Change
				wait(2);					//Wait for 2 periods 1.04 seconds
			}
			else{							//Small Pressure Change
				wait(1);					//Wait for 1 period 0.52 seconds
			}

			//Ensure that the boost converter is off and both valves are off
			P2OUT &= ~BIT2;
			P2OUT &= ~BIT1;
			P2OUT &= ~BIT0;

			wait(2);						//Wait for 1.04 seconds to allow stabilization

			previousPressure = currentPressure;
			getPressure();
			currentPressure = (RxBuffer[0] << 8) | RxBuffer[1];
			postPressure();
			sendUART();
			getValuesUART();

		}
		if(baseCase){
			wait(1);
		}
	}
}

// USCI A0/B0 Transmit ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCI0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	if (IFG2 & UCB0RXIFG){
		TACTL = TACLR + MC_0;           			// SMCLK, contmode, interrupt
		RXByteCtr--;                              // Decrement RX byte counter
		if (RXByteCtr){
			*PRxData++ = UCB0RXBUF;                 // Move RX data to address PRxData
			if (RXByteCtr == 1){                     // Only one byte left?
				UCB0CTL1 |= UCTXSTP;                  // Generate I2C stop condition
			}
		}
		else{
			*PRxData = UCB0RXBUF;                   // Move final RX data to PRxData
			__bic_SR_register_on_exit(CPUOFF);      // Exit LPM0
		}
	}

	else if (IFG2 & UCA0TXIFG){
		TACTL = TACLR + MC_0;           			// SMCLK, contmode, interrupt
		UCA0TXBUF = UART_TX[i++];                 // TX next character
		if (i == sizeof UART_TX){                  // TX over?
			IE2 &= ~UCA0TXIE;                       // Disable USCI_A0 TX interrupt
			TACTL = TASSEL_2 + ID_3 + MC_2 + TAIE;           // SMCLK, contmode, interrupt
			IE2 |= UCA0RXIE;                        // Enable USCI_A0 RX interrupt
		}
	}
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
	TACTL = TACLR + MC_0;           			// SMCLK, contmode, interrupt
	UART_RX[j++] = UCA0RXBUF;
	if (j > sizeof UART_RX - 1){
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
 switch( TA0IV ){
	case  2: break;                          			// CCR1 not used
	case  4: break;                          			// CCR2 not used
	case 10:
		TACTL = TACLR + MC_0;           			// SMCLK, contmode, interrupt
		__bic_SR_register_on_exit(CPUOFF);          // overflow Exit LPM0
		break;
	}
}

void getPressure(void){
	//I2C Portion
	PRxData = (unsigned char *)RxBuffer;    // Start of RX buffer
	RXByteCtr = 2;                          // Load RX byte counter
	while (UCB0CTL1 & UCTXSTP);             // Ensure stop condition got sent
	UCB0CTL1 |= UCTXSTT;                    // I2C start condition
	TACTL = TASSEL_2 + ID_3 + MC_2 + TAIE;  // SMCLK, contmode, interrupt
	__bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
											// Remain in LPM0 until all data
											// is RX'd
	TACTL = TACLR + MC_0;           		// SMCLK, contmode, interrupt
}

void sendUART(void){
	  //UART Portion
	  i = 0;
	  j = 0;
	  IE2 |= UCA0TXIE;                        // Enable USCI_A0 TX interrupt
	  TACTL = TASSEL_2 + ID_3 + MC_2 + TAIE;  // SMCLK, contmode, interrupt
	  __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled
	  TACTL = TACLR + MC_0;           			// SMCLK, contmode, interrupt

	  //Wait for 2 cycles of .52 seconds
	  //wait(2);				// This may be needed
}

void postPressure(void){
	msbPressure = RxBuffer[0];
	if (((msbPressure & 128) && (msbPressure & 64)) || currentPressure == 0){
		StatusByte |= SensorFailure;
	}
	else{
		StatusByte &= ~SensorFailure;
	}

	UART_TX[0] = RxBuffer[1];
	UART_TX[1] = RxBuffer[0];
	UART_TX[2] = StatusByte;
}

void getValuesUART(void){
	if(UART_RX[0] || UART_RX[1]){
		// Check to see if the data sent shifted one over to the right by checking the error check for shift of FF
		if(UART_RX[0] == 0xFF){
			desiredPressure = (UART_RX[2] << 8 | UART_RX[1]);
			newStatus = UART_RX[3];
		}
		else{
			desiredPressure = (UART_RX[1] << 8) | UART_RX[0];
			newStatus = UART_RX[2];
		}
	}
	if(newStatus & newCO2){
		StatusByte &= ~lowCO2;
	}
	if(newStatus & ForceIdle){
		desiredPressure = currentPressure;
	}
}

void wait(unsigned int waitCount){
	int x;
	for(x = 0; x < waitCount; x++){
		TACTL = TASSEL_2 + ID_3 + MC_2 + TAIE;      // SMCLK, contmode, interrupt
		__bis_SR_register(LPM0_bits + GIE);         // Enter LPM0 w/ interrupt
		TACTL = TACLR + MC_0;           			// SMCLK, contmode, interrupt
	}
}
