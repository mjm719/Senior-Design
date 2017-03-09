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

//Variables for data that is being transferred
unsigned char PressureRX[2];
unsigned char *PPressureRX;         //Pointer to PressureRX
unsigned char BlueRX[2];
unsigned char *PBlueRX;         	//Pointer to BlueRX
unsigned char TXData;				//Data going to BlueTooth
unsigned char PressureData0;		//LSB of Pressure Data
unsigned char PressureData1;		//Bits 6 and 7 are status bits, MSB of Pressure Data
unsigned char DesiredPressure;		//Pressure the user wants the tire pressure to be set to
unsigned char StatusByte;			//Status Information to be sent back to the user's Android application

//StatusByte Layout
//Bit		7			6		5			4				3			2			1			0
//Flag		?????		????	????????	????????????	????????	????????	????????	SensorFailure
//Bit Val	128			64		32			16				8			4			2			1

//Possibly front/back flag, power on/ off flag

//CO2 empty
//Possible Tire Leak


//To Turn Bit On 	StatusByte |= Flag
//To Turn Bit Off	StatusByte &= ~Flag
//To Check if Bit is On	if (StatusByte & Flag)
//To Check if Bit is Off	if (StatusByte ^ Flag)
//Ex. To Turn SensorFailure On		StatusByte |= SensorFailure;
//If a flag is on, that indicates there was a failure with that flag

//StatusByte Flags / Bits of StatusByte
const unsigned char SensorFailure = 1;

//*************************************************************************************//
//IMPORTANT TESTING NOTICE: All VirtualBench Data has been shifted right once
//*************************************************************************************//

unsigned char TXByteCtr;			//Variable to determine how many more bytes of data you have left to send
unsigned char RXByteCtr;			//Variable to determine how many more bytes of data you have left to read
unsigned char RorW;					//Variable to tell the master in the the interrupt whether to write or read
unsigned char I2CAction;			//Variable to determine what I2C action to take 0x01 is read from pressure sensor, 0x02 is read from BlueTooth, 0x03 is write to BlueTooth
unsigned char SlaveAddress;			//Determines which slave device to communicate with over I2C

void SetupPressureRX(void);
void SetupRX(void);
void SetupUART(void);

int main(void){

	//Stop Watchdog Timer and Reset all pins
	WDTCTL = WDTPW + WDTHOLD;                 //Stop watch dog timer
	P1DIR = 0xFF;                             //All P1.x outputs
	P1OUT = 0;                                //All P1.x reset
	P2DIR = 0xFF;                             //All P2.x outputs
	P2OUT = 0;                                //All P2.x reset
	P3DIR = 0xFF;                             //All P3.x outputs
	P3OUT = 0;                                //All P3.x reset

	//Clock Management, mostly for UART
	DCOCTL = 0;                               // Select lowest DCOx and MODx settings
	BCSCTL1 = CALBC1_1MHZ;                    // Set DCO
	DCOCTL = CALDCO_1MHZ;

	//Initialize Pins to be used
	P1SEL |= BIT6 + BIT7;                     //Assign I2C pins to USCI_B0, P1.6 is clock line, P1.7 is data line
	P1SEL2|= BIT6 + BIT7;                     //Assign I2C pins to USCI_B0
	//P1SEL = BIT1 + BIT2 + BIT4;               //P1.1 = RXD, P1.2=TXD
	//P1SEL2 = BIT1 + BIT2;                     //P1.4 = SMCLK, others GPIO

	//Set the device up for UART
	//SetupUART();

	P1OUT &= ~BIT0;                           // P1.0 = 0
	P1DIR |= BIT0;                            // P1.0 output

	//Setup P2.1 as output for ??? and set low
	P2SEL &= (~BIT1);
	P2DIR |= BIT1;
	P2OUT &= ~BIT1;
	//unsigned char x = 1;
	while (1){

		//UC0IE |= UCA0TXIE; // Enable USCI_A0 TX interrupt
		__delay_cycles(1000);
		// Read the value of the current tire pressure from the pressure sensor
		SetupPressureRX();
	    while (UCB0CTL1 & UCTXSTP);             //Check to see if the line is currently open
	    UCB0CTL1 |= UCTXSTT;                    //UCTXSTT sends the start bit reserving communication on the line between the master and slave
												//UCTXSTT also sends the address that is currently being stored in UCB0I2CSA, which starts the communication and reserves the line
	    while (UCB0CTL1 & UCTXSTT);             //Check to see if the master and slave have opened the I2C line for communication
	    UCB0CTL1 |= UCTXSTP;                    //Send a stop condition and wait for the slave that was addressed to respond with the data you want to read
	    //__bis_SR_register(CPUOFF + GIE);      //Enter LPM0 w/ interrupts

	    //Store Data into variables for use
	    PressureData0 = PressureRX[0];			//
	    PressureData1 = PressureRX[1];			//

	    // Check data from Pressure Sensor and Convert to a usable number
	    if ((PressureData1 & 128) && (PressureData1 & 64)){		//If both status bits, 6 and 7, are 1, then an error occurred, in this case
	    	//There is an error in the pressure sensor, the user needs to replace it
	    	StatusByte |= SensorFailure;		//Turn on SensorFailure in the StatusByte to be sent over BlueTooth to the app
	    }
	    else{
	    	//Need to determine if we want to convert the data before sending to BlueTooth or not
	    	//TXData = ;
	    }
	    //__bis_SR_register(CPUOFF + GIE);      //Enter low power mode and wait for an interrupt, interrupts will still be serviced
												//Remain in LPM0 until all data
												//is TX'd
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
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCIAB0TX_ISR (void)			//Interrupt Service routine to be handled
#else																			//
#error Compiler not supported!
#endif																			//
{
	if (I2CAction == 0x01){							//Determine if master is set to read from the pressure sensor
		RXByteCtr--;
		if (RXByteCtr){
			*PPressureRX++ = UCB0RXBUF;             //Move RX data to address PPressureRX
			if (RXByteCtr == 1){                    //Only one byte left?
			  UCB0CTL1 |= UCTXSTP;                  //Generate I2C stop condition
			}
			IFG2 |= UCB0RXIFG;
		}
		else{
			*PPressureRX = UCB0RXBUF;               //Move final RX data to PPressureRX
			__bic_SR_register_on_exit(CPUOFF);      //Exit LPM0
			//OperationComplete = TRUE;				//Probably won't be needed since using LPMO
			//PressureData = UCB0RXBUF;             //**********Store data you are reading from the slave into PressureData
			//TXData = PressureData;				//Set what is being sent back to BlueTooth as the pressure sensor data
		}
	}
}

// Echo back RXed character, confirm TX buffer is ready first
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	//if (IFG2 & UCA0RXIFG){
	//	P1OUT |= BIT0;                    // P1.0 = 1
	//}
	while (!(IFG2&UCA0TXIFG));              //USCI_A0 TX buffer ready?
	//unsigned char array[] = {'T', 'E', 'S', 'T'};
	UCA0TXBUF = 1;	//UCA0RXBUF;                  //TX -> RXed character
}

//Function sets MSP430 up to use I2C in order to communicate with the pressure sensor
void SetupPressureRX(void){
	SlaveAddress = 0x28;						//Base address is 0x28, to read, it should be 0x29
	I2CAction = 0x01;							//Sets I2C action to read from pressure sensor
	RXByteCtr = 2;								//Set to read 2 bytes of data
	PPressureRX = (unsigned char *)PressureRX;  //Start of RX buffer
	SetupRX();									//Function call to setup the master reading from the slave
}

//Function sets the master device up to receive RX interrupts and prepare for reading data
void SetupRX(void){
	__disable_interrupt();					//Clears interrupts that have been enabled
	RorW = 1;								//Sets the master to read when in interrupts
	IE2 &= ~UCB0TXIE;						//Hard coded to ensure that TX interrupts are no longer enabled
	IFG2 &= ~UCA0RXIFG;						//Hard coded to ensure that the RX interrupt flag is cleared
	while (UCB0CTL1 & UCTXSTP);             //Make sure the line is available to communicate over
	UCB0CTL1 |= UCSWRST;                    //Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;   //I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;          //Use SMCLK, keep SW reset
	UCB0BR0 = 12;                           //fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = SlaveAddress;               //This is the slave to be addressed
	UCB0CTL1 &= ~UCSWRST;                   //Clear SW reset, resume operation
	IE2 |= UCB0RXIE;                        //Enable RX interrupt, this will allow you to read data
}

//Function Prepares the MSP430 to use UART
void SetupUART(void){
	UCA0CTL1 |= UCSSEL_2;                     //SMCLK
	UCA0BR0 = 8;                              //1MHz 115200
	UCA0BR1 = 0;                              //1MHz 115200
	UCA0MCTL = UCBRS2 + UCBRS0;               //Modulation UCBRSx = 5
	UCA0CTL1 &= ~UCSWRST;                     //**Initialize USCI state machine**
	IE2 |= UCA0TXIE;                          //Enable USCI_A0 RX interrupt
}
