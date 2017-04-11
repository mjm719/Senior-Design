//******************************************************************************
//	--Mike McConville--
//
//	This file is to be used for a Slave MSP430 talking to a Master MSP430
//	via I2C in both write and read modes. This file is the slave portion of
//	the code. This code also obtains ADC measurements, handles IO pins, timers,
//  and a control structure of the code that is to be used on the slave side
//  of Bill's project. This file will be loaded onto 6 different slave MSP430s.
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

// Function Prototypes
void SetupADC(void);

int reading;
#define shutOffVoltage 885		//for a 3.3V reference, 885 or lower ADC value should shut off the system. 885 correlates to a 2.858V battery


//******* MAIN FUNCTION *******//

int main(void){
	WDTCTL = WDTPW + WDTHOLD;           	// Stop WDT

	// Set DCO CLK Source to 1MHz
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	// Setup P2.3 as output for system power and set low
	P2SEL &= (~BIT3);
	P2DIR |= BIT3;
	P2OUT &= ~BIT3;

	while (1){

		// Reset Watchdog Timer to 0
		//WDTCTL = WDTPW + WDTCNTCL;				// Resets watchdog, will timeout in 32.768ms unless reset

		SetupADC();	//865
		ADC10CTL0 &= ~ENC;
		while (ADC10CTL1 & BUSY);               			// Wait if ADC10 core is active
		ADC10CTL0 |= ENC + ADC10SC;             			// Sampling and conversion start
		reading = ADC10MEM;
		__delay_cycles(1000000);
		if(reading <= shutOffVoltage){
			P2OUT |= BIT3;
		}
		else{
			P2OUT &= ~BIT3;
		}
	}
}

//******* SETUP FUNCTIONS *******//

// Setup the ADC pins that will be used to measure the voltages and currents on the board
void SetupADC(void){
	ADC10CTL0 = SREF1 + ADC10SHT_2 + ADC10ON;	// Turn on ADC for 8 sample and hold time with external refernce;
	ADC10CTL1 = INCH_0;							// Sample A0
	ADC10AE0 |= 0x01;							// P1.0 ADC10 option select
}
