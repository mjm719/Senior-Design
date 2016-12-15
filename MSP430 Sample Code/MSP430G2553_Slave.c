//******************************************************************************
//	--Mike McConville--
//
//	This file is the to be used for a Slave MSP430 talking to a Master MSP430
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

// Definitions
#define LOW 		0
#define HIGH 		1

// Bits of StatusWord
unsigned char StatusWord_Byte2 = 0x00;
unsigned char StatusWord_Byte1 = 0x00;

const unsigned char OverLoadFlag = 16;
const unsigned char NoLoadFlag = 8;
const unsigned char VSetFlag = 4;
const unsigned char PulseDetFlag = 2;
const unsigned char RLoopFlag = 1;
const unsigned char GfiFlag = 128;
const unsigned char OvpFlag = 64;
const unsigned char CurFlag = 32;
const unsigned char CurSenseFault = 16;
const unsigned char ResendFlag = 8;
const unsigned char VChangeFlag = 4;
const unsigned char PulseLevel = 2;
const unsigned char PatternFlag = 1;

// StatusWord_Byte2
// Bit		7	6	5	4				3			2			1				0
// Flag		0	0	0	OverLoadFlag	NoLoadFlag	VSetFlag	PulseDetFlag	RLoopFlag
// Bit Val				16				8			4			2				1

// StatusWord_Byte1
// Bit		7			6		5			4				3			2			1			0
// Flag		GfiFlag		OvpFlag	CurFlag	CurSenseFault	ResendFlag	VChangeFlag	PulseLevel	PatternFlag
// Bit Val	128			64		32			16				8			4			2			1

// To Turn Bit On 	StatusWord_ByteX |= Flag
// To Turn Bit Off	StatusWord_ByteX &= ~Flag
// To Check if Bit is On	if (StatusWord_ByteX & Flag)
// To Check if Bit is Off	if (StatusWord_ByteX ^ Flag)
// ex. To Turn ResendFlag On	StatusWord_Byte1 |= ResendFlag;


// Constants
#define V40 		50				// 40 Volts on Vsense scale
#define V10			12				// 10 Volts on Vsense scale
#define GfiValue	682
#define A1m			14				// 0.001 Amps
#define ILIML 		146				// 0.010 Amps
#define ILIMH 		866				// 0.059 Amps
#define ITHRES1 	29				// 0.002 Amps
#define ITHRES2		44				// 0.003 Amps
const unsigned char PATTERN[] = {23, 10, 23, 23, 10, 23, 10, 10, 23, 10};

// Global Variables
unsigned int ILim = ILIML;

// Pulse Time Measurement Variables
unsigned char Tinit = 0;
unsigned char TPulseLength[10];
unsigned int Iout1 = 0;
unsigned int Iout2 = 0;
unsigned int Iout3 = 0;
unsigned char PulseCount = 0;
unsigned int Cur1_Measurement_Time;
unsigned int T0 = 0;
unsigned int T1;
unsigned int T2;

// ADC Measurement Variables
unsigned int IgfiMeas;				// Scale Factor 1/58.8 V/V, 	Pin P1.1
unsigned int CurMeas1;				// Scale Factor 1/21.5 V/mA, 	Pin P1.2
unsigned int CurMeas2;				// Scale Factor 1/21.5 V/mA, 	Pin P1.3
unsigned int Vsense;				// Scale Factor 1/366 V/V, 		Pin P1.5
unsigned int ADC_Measurements[4];	// Used as address fro ADC10SA to transfer data to

// I2C Variables
unsigned char I2C_Action = 0x00;	// Initially set to wait for receive instruction
unsigned char I2C_Address;
unsigned char RXByteCtr;
unsigned char TXByteCtr;
unsigned char IO_Byte;
unsigned char TempV_msb;
unsigned char TempV_lsb;
unsigned char TempOvp_msb;
unsigned char TempOvp_lsb;
unsigned char Status_Array[3];
unsigned char Readings_Array[12];
unsigned char *PTxStatus; 	                // Pointer to Status Array
unsigned char *PTxReadings;                 // Pointer to Readings Array

// Voltage Change Variables
unsigned int TempOvp = 0;
unsigned int TempV = 0;
unsigned int TempDelta = 0;
unsigned int VOvp = 220;
unsigned int Vreg = 200;

// Output Variables and initialize to LOW, only used to keep track of output pins' states
unsigned char HVCurEn = LOW;				// Pin P2.1
unsigned char VoltRange = LOW;				// Pin P2.2
unsigned char Shutdown = LOW;				// Pin P2.3

// Input Variables
#define HVCurDet	P2IN					// Pin P2.0

// Function Prototypes
void SetupADC(char ADC_count);
void SetupI2C(void);
void Setup_IO_Byte(void);
void Setup_Status_Array(void);
void Setup_Readings_Array(void);
void Find_I2C_Address(void);
void Measure_ADC_Pins(char ADC_count);
void Detect_Pulse_Sequence(void);
void Check_Pulse_vs_Pattern(void);
void Check_Overload_noLoad(void);

//******* MAIN FUNCTION *******//

int main(void){

	WDTCTL = WDTPW + WDTHOLD;           	// Stop WDT

	// Set DCO CLK Source to 8MHz and SMCLK to 1MHz
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;
	BCSCTL2 = DIVS_3;							// Set the SMCLK to 1MHz

	// Setup P2.1 as output for HvCurEn and set low
	P2SEL &= (~BIT1);
	P2DIR |= BIT1;
	P2OUT &= ~BIT1;

	// Setup P2.2 as output for VoltRange and set low
	P2SEL &= (~BIT2);
	P2DIR |= BIT2;
	P2OUT &= ~BIT2;

	// Setup P2.3 as output for Shutdown and set low
	P2SEL &= (~BIT3);
	P2DIR |= BIT3;
	P2OUT &= ~BIT3;

	// Setup I2C pins
	P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0

	// Local Variables
	unsigned int Imax = 0;
	unsigned char CurCount1 = 0;
	unsigned char CurCount2 = 0;
	unsigned char GfiCount = 10;
	unsigned char OvpCount = 0;
	unsigned int SumA = 0;
	unsigned int SumB = 0;
	unsigned char RLoopCount = 0;
	unsigned int CURA4 = 0;
	unsigned int CURA3 = 0;
	unsigned int CURA2 = 0;
	unsigned int CURA1 = 0;
	unsigned int CURB4 = 0;
	unsigned int CURB3 = 0;
	unsigned int CURB2 = 0;
	unsigned int CURB1 = 0;
	unsigned char ADC_count = 0;

	// Enable Watchdog Timer, Call to Setup Functions, and Start Timer
	WDTCTL = WDTPW + WDTCNTCL;					// Watchdog will timeout every 32.768ms unless reset
	SetupI2C();
	TA0CTL |= TASSEL_2 + MC_2 + TACLR;       	// SMCLK, Cont Mode, Start Timer, Timer used to measure pulse period
	Tinit = TA0R;								// Find the intial time the while loop started running
	TPulseLength[0] = Tinit;					// Store the initial time in the Pulse length array

	//******* WHILE LOOP *******//

	while (1){

		// Reset Watchdog Timer to 0
		WDTCTL = WDTPW + WDTCNTCL;				// Resets watchdog, will timeout in 32.768ms unless reset

		// Make sure ADC_count
		ADC_count = 0;

		// Measure Input Pins by calling functions
		Measure_ADC_Pins(ADC_count);

		// Enable Global Interupts for I2C functionality
		__bis_SR_register(GIE);        			// Global Interupts Enabled

		// Check Vsense against VOvp
		if (Vsense > VOvp){
			Measure_ADC_Pins(3);
			if (Vsense > VOvp){
				StatusWord_Byte1 |= OvpFlag;	// Set OvpFlag HIGH
				P2OUT |= BIT3;					// Physically set Shutdown high
				Shutdown = HIGH;				// Set shutdown high in software
				// Need to power reset in this situation
				break;							// Ends Execution by breaking out of while
			}
		}

		// Check the output VoltRange
		if (VoltRange == LOW){
			if (Vsense > V40 + V10){
				OvpCount++;
				if (OvpCount > 10){
					StatusWord_Byte1 |= OvpFlag;	// Set OvpFlag HIGH
					P2OUT |= BIT3;				// Physically set Shutdown high
					Shutdown = HIGH;			// Set shutdown high in software
					// Need to power reset in this situation
					break;						// Ends Execution by breaking out of while
				}
			}
			else {
				OvpCount = 0;
			}
		}
		else {
			OvpCount = 0;
		}


		// Check CurMeas1's value against ILim
		if (CurMeas1 > ILim + A1m){
			CurCount1++;
		}
		else{
			CurCount1 = 0;
		}

		// Check if CurMeas1 is still too high
		if (CurCount1 > 10){
			StatusWord_Byte1 |= CurFlag;			// Set CurFlag HIGH;
			P2OUT |= BIT3;							// Physically set shutdown high
			Shutdown = HIGH;						// Set shutdown high in software
			// Need to power reset in this situation
			break;									// Ends Execution by breaking out of while
		}

		// Check CurMeas2's value against ILim
		if (CurMeas2 > ILim + A1m){
			CurCount2++;
		}
		else{
			CurCount2 = 0;
		}

		// Check if CurMeas2 is still too high
		if (CurCount2 > 10){
			StatusWord_Byte1 |= CurFlag;			// Set CurFlag HIGH;
			P2OUT |= BIT3;							// Physically set shutdwon high
			Shutdown = HIGH;						// Set shutdown high in software
			// Need to power reset in this situation
			break;									// Ends Execution by breaking out of while
		}

		// Check for a ground fault
		if (IgfiMeas > GfiValue){
			GfiCount++;
			// Determine if the ground fault continues to exist for multiple cycles
			if (GfiCount > 10){
				StatusWord_Byte2 |= GfiFlag;			// Set GfiFlag HIGH;
			}
		}
		else{
			StatusWord_Byte2 &= ~GfiFlag;			// Set GfiFlag LOW;
			if (GfiCount <= 2){
				GfiCount = 0;						// Reset count to 0
			}
			else {
				GfiCount -= 2;
			}
		}

		// Changes the variables to match what the master has changed the output voltage to be
		if (StatusWord_Byte1 & VChangeFlag){			// Check if VChangeFlag is HIGH
			if (TempOvp = TempV + TempDelta){
				VOvp = TempOvp;
				Vreg = TempV;
				StatusWord_Byte2 |= VSetFlag;			// Set VSetFlag HIGH;
				StatusWord_Byte1 &= ~VChangeFlag;		// Set VChangeFlag LOW;
				StatusWord_Byte1 &= ~ResendFlag;		// Set ResendFlag LOW;
			}
			else{
				StatusWord_Byte1 |= ResendFlag;			// Set ResendFlag HIGH;
			}
		}

		// Average together current readings for Current 1
		CURA4 = CURA3;
		CURA3 = CURA2;
		CURA2 = CURA1;
		CURA1 = CurMeas1;
		SumA = (CURA4 + CURA3 + CURA2 + CURA1) >> 2;

		// Average together current readings for Current 2
		CURB4 = CURB3;
		CURB3 = CURB2;
		CURB2 = CURB1;
		CURB1 = CurMeas2;
		SumB = (CURB4 + CURB3 + CURB2 + CURB1) >> 2;

		// Reject any currents over 10mA
		if ((CurMeas1 > ILIML) || (CurMeas2 > ILIML)){
			SumA = 0;
			SumB = 0;
		}

		// Make sure the currents are similar values
		if ((SumA > (SumB + A1m)) || (SumA < (SumB - A1m))){
			StatusWord_Byte1 |= CurSenseFault;			// Set CurSenseFault HIGH;
		}
		else {
			StatusWord_Byte1 &= ~CurSenseFault;			// Set CurSenseFault LOW;
		}

		//Determine if RLoop is High
		if (StatusWord_Byte2 & RLoopFlag){				// See if RLoopFlag is HIGH
			Detect_Pulse_Sequence();					// Finish remaining code in function
		}
		else {
			// Determine if SumA or SumB is greater than Imax and set it to the new Imax
			if ((SumA >= Imax) && (SumA >= SumB)){
				Imax = SumA;
			}
			else if ((SumB >= Imax) && (SumB >= SumA)){
				Imax = SumB;
			}
			// Determine if the currents are in range
			if ((Imax > ITHRES1) || (StatusWord_Byte2 & PulseDetFlag)){		// See if PulseDetFlag is HIGH
				StatusWord_Byte2 |= PulseDetFlag;							// Set PulseDetFlag HIGH;
				if (Imax > ITHRES2){
					RLoopCount++;
					if (RLoopCount > 10){
						Imax = 0;
						StatusWord_Byte2 |= RLoopFlag;				// Set RLoopFlag HIGH;
						StatusWord_Byte2 &= ~PulseDetFlag;			// Set PulseDetFlag LOW;
						Detect_Pulse_Sequence();					// Finish remaining code in function
					}
				}
			}
		}
	}
	// This while loop will only ever be entered if shutdown is enabled. This loop is here so all the code
	// will do is wait for I2C to send back something to the master indicating a problem.
	WDTCTL = WDTPW + WDTHOLD;           		// Stop WDT
	while (1){
		__bis_SR_register(GIE);        			// Global Interupts Enabled
	}
}


//******* INTERUPT SERVICE ROUTINES *******//

// USCI_B0 Data ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCIAB0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{
	//***** Receive Section *****//
	if (IFG2 & UCB0RXIFG){
		// Determines which instuction you are getting from the master
		if (I2C_Action == 0x00){
			I2C_Action = UCB0RXBUF;
			if (I2C_Action == 0x01){				// Prepares for Reading the voltage change variables
				RXByteCtr = 5;
			}
			else if (I2C_Action == 0x02){			// Prepares for sending status word to master
				TXByteCtr = 0;
			}
			else if (I2C_Action == 0x03){			// Prepares for sending ADC info back to master
				TXByteCtr = 0;
			}
			IFG2 &= ~UCA0RXIFG;
		}

		// Reads the voltage change info from the master and resets the variables to match new voltage
		else if (I2C_Action == 0x01){
			if (RXByteCtr == 5){					// Read the TempOvp variable, Byte2
				StatusWord_Byte1 |= VChangeFlag;	// Set VChangeFlag HIGH;
				TempOvp_msb = UCB0RXBUF;
			}
			else if (RXByteCtr == 4){				// Read the TempOvp variable, Byte1
				TempOvp_lsb = UCB0RXBUF;
				TempOvp = (TempOvp_msb << 8) | TempOvp_lsb;			// Combines the individual bytes to make a 2 byte variable
			}
			else if (RXByteCtr == 3){				// Read the TempV variable, Byte2
				TempV_msb = UCB0RXBUF;
			}
			else if (RXByteCtr == 2){				// Read the TempV variable, Byte1
				TempV_lsb = UCB0RXBUF;
				TempV = (TempV_msb << 8) | TempV_lsb;			// Combines the individual bytes to make a 2 byte variable
			}
			else if (RXByteCtr == 1){				// Read the TempDelta variable
				TempDelta = UCB0RXBUF;
				I2C_Action = 0x00;					// Set to wait for read state
				IFG2 &= ~UCA0RXIFG;
			}
			RXByteCtr--;
		}
		else {
			I2C_Action = 0x00;					// Set to wait for read state
			UCB0CTL1 |= UCTXNACK;
		}
	}

	//***** Transfer Section *****//
	else {
		// Sends the Status Word info back to the master along with input and output variables
		if (I2C_Action == 0x02){
			if (TXByteCtr == 0){
				Setup_Status_Array();
			}
			UCB0TXBUF = *PTxStatus++;           // Transmit data at address PTxData
			TXByteCtr++;                        // Increment TX byte counter
		}

		// Send ADC numbers back to the master
		else if (I2C_Action == 0x03){
			if (TXByteCtr == 0){
				Setup_Readings_Array();
			}
			UCB0TXBUF = *PTxReadings++;         // Transmit data at address PTxData
			TXByteCtr++;                        // Increment TX byte counter
		}
		else {
			I2C_Action = 0x00;					// Set to wait for read state
			UCB0CTL1 |= UCTXNACK;
		}

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
	UCB0STAT &= ~(UCSTPIFG + UCSTTIFG);       // Clear interrupt flags
	if (TXByteCtr & (I2C_Action > 0x01)){     // Check TX byte counter, but only do so when you are have an I2C transaction that transfers
		I2C_Action = 0x00;					  // Set to wait for read state
	}
}


//******* SETUP FUNCTIONS *******//

// Setup the ADC pins that will be used to measure the voltages and currents on the board
void SetupADC(char ADC_count){
	ADC10CTL0 = SREF1 + ADC10SHT_2 + ADC10ON;	// Turn on ADC for 8 sample and hold time with external refernce;
	if (ADC_count == 0){
		ADC10CTL1 = INCH_0;							// Sample A0
		ADC10AE0 |= 0x01;							// P1.0 ADC10 option select
	}
	else if (ADC_count == 1){
		ADC10CTL1 = INCH_2;							// Sample A2
		ADC10AE0 |= 0x04;							// P1.2 ADC10 option select
	}
	else if (ADC_count == 2){
		ADC10CTL1 = INCH_3;							// Sample A3
		ADC10AE0 |= 0x08;							// P1.3 ADC10 option select
	}
	else if (ADC_count == 3){
		ADC10CTL1 = INCH_5;							// Sample A5
		ADC10AE0 |= 0x20;							// P1.5 ADC10 option select
	}
}

// Function sets up the slave device for I2C communication
void SetupI2C(void){
	Find_I2C_Address();
	UCB0CTL0 = UCMODE_3 + UCSYNC;             		// I2C Slave, synchronous mode
	UCB0I2COA = I2C_Address;                    	// Own Address is 048h
	UCB0CTL1 &= ~UCSWRST;                     		// Clear SW reset, resume operation
	UCB0I2CIE |= UCSTTIE;                     		// Enable STT interrupt
	IE2 |= UCB0TXIE + UCB0RXIE;               		// Enable TX and RX interrupts
}

// Sets up a byte with the status of the input and output variables for I2C transfer
void Setup_IO_Byte(void){
	IO_Byte = 0x00;							// Bits 4-7 are always 0
	if (HVCurEn == HIGH){					// Bit 3 = HVCurEn
		IO_Byte += 8;
	}
	if (VoltRange == HIGH){					// Bit 2 = VoltRange
		IO_Byte += 4;
	}
	if (Shutdown == HIGH){					// Bit 1 = Shutdown
		IO_Byte += 2;
	}
	if (HVCurDet & BIT0){					// Bit 0 = HVCurDet
		IO_Byte++;
	}
}

// This functions sets up the data in Status Array to be sent to the master
void Setup_Status_Array(void){
	Setup_IO_Byte();
	Status_Array[0] = 0x11;//StatusWord_Byte2;
	Status_Array[1] = 0x02;//StatusWord_Byte1;
	Status_Array[2] = 0x03;//IO_Byte;
	PTxStatus = (unsigned char *)Status_Array;      // Start of TX buffer
}

// This functions sets up the data in Readings Array to be sent to the master
void Setup_Readings_Array(void){
	Readings_Array[0] = (IgfiMeas >> 8) & 0xFF;			// Set the MSB of IgfiMeas
	Readings_Array[1] = (IgfiMeas & 0xFF);				// Set the LSB of IgfiMeas
	Readings_Array[2] = (CurMeas1 >> 8) & 0xFF;			// Set the MSB of CurMeas1
	Readings_Array[3] = (CurMeas1 & 0xFF);				// Set the LSB of CurMeas1
	Readings_Array[4] = (CurMeas2 >> 8) & 0xFF;			// Set the MSB of CurMeas2
	Readings_Array[5] = (CurMeas2 & 0xFF);				// Set the LSB of CurMeas2
	Readings_Array[6] = (Vsense >> 8) & 0xFF;			// Set the MSB of Vsense
	Readings_Array[7] = (Vsense & 0xFF);				// Set the LSB of Vsense
	Readings_Array[8] = (Vreg >> 8) & 0xFF;				// Set the MSB of Vreg
	Readings_Array[9] = (Vreg & 0xFF);					// Set the LSB of Vreg
	Readings_Array[10] = (VOvp >> 8) & 0xFF;			// Set the MSB of VOvp
	Readings_Array[11] = (VOvp & 0xFF);					// Set the LSB of VOvp
	PTxReadings = (unsigned char *)Readings_Array;      // Start of TX buffer
}

//******* SUB-ROUTINE FUNCTIONS *******//

// Finds the unique address of the slave device by looking at P3.0, P3.1, P3.2
void Find_I2C_Address(void){
	  if (P3IN == 0x07){
		  I2C_Address = 0x27;
	  }
	  else if (P3IN == 0x06){
		  I2C_Address = 0x26;
	  }
	  else if (P3IN == 0x05){
		  I2C_Address = 0x25;
	  }
	  else if (P3IN == 0x04){
		  I2C_Address = 0x24;
	  }
	  else if (P3IN == 0x03){
		  I2C_Address = 0x23;
	  }
	  else if (P3IN == 0x02){
		  I2C_Address = 0x22;
	  }
	  else if (P3IN == 0x01){
		  I2C_Address = 0x21;
	  }
	  else if (P3IN == 0x00){
		  I2C_Address = 0x20;
	  }
}

// Read each ADC pin and store the result in the respective variable for each pin
void Measure_ADC_Pins(char ADC_count){
	for (ADC_count = 0; ADC_count < 4; ADC_count++){
		SetupADC(ADC_count);
		ADC10CTL0 &= ~ENC;
		while (ADC10CTL1 & BUSY);               			// Wait if ADC10 core is active
		ADC10CTL0 |= ENC + ADC10SC;             			// Sampling and conversion start
		if (ADC_count == 1){
			Cur1_Measurement_Time = TA0R;					// Used later to determine what time pulse went high or low
		}
		ADC_Measurements[ADC_count] = ADC10MEM;
	}
	// Set the ADC variables by grabbing them from the ADC_Measurements array
	IgfiMeas = ADC_Measurements[0];
	CurMeas1 = ADC_Measurements[1];
	CurMeas2 = ADC_Measurements[2];
	Vsense = ADC_Measurements[3];
}

// Continuation of Main, Detects the pulse sequence and measures the pulse time
void Detect_Pulse_Sequence(void){
	if ((StatusWord_Byte1 & PatternFlag) && (PulseCount >= 10)){		// See if PatternFlag is HIGH
		Check_Overload_noLoad();
	}
	else{
		Iout3 = Iout2;
		Iout2 = Iout1;
		Iout1 = CurMeas1;
		// Determine if PulseLevel is high
		if (StatusWord_Byte1 & PulseLevel){			// See if PulseLevel is HIGH
			// Determine if all 3 currents are below ITHRES1
			if ((Iout3 < ITHRES1) && (Iout2 < ITHRES1) && (Iout1 < ITHRES1)){
				T2 = Cur1_Measurement_Time;			// Measure time pulse goes low and save as T2
				StatusWord_Byte1 &= ~PulseLevel;	// Set PulseLevel LOW;
				TPulseLength[PulseCount] = T2 - T1;
				T0 = T2;
				Check_Pulse_vs_Pattern();
			}
		}
		else{
			// Determine if all 3 currents are above ITHRES2
			if ((Iout3 > ITHRES2) && (Iout2 > ITHRES2) && (Iout1 > ITHRES2)){
				T1 = Cur1_Measurement_Time;			// Measure time pulse goes high and save as T1
				StatusWord_Byte1 |= PulseLevel;		// Set PulseLevel HIGH;
				// See if the transistion from high to low or vice versa is occuring quick enough
				if (T1 - T0 > 20000){				// 20000 SMCLK ticks and Timer Ticks is 20ms
					PulseCount = 0;
					StatusWord_Byte1 |= PatternFlag;		// Set PatternFlag HIGH;
				}
				else {
					PulseCount++;
					TPulseLength[PulseCount] = (T1 - T0);
					Check_Pulse_vs_Pattern();
				}
			}
		}
	}
}

// Checks the Pulse value against the Pattern value with +- 2 range of tolerance
void Check_Pulse_vs_Pattern(void){
	if ((TPulseLength[PulseCount] < PATTERN[PulseCount] - 2) ||
			(TPulseLength[PulseCount] > PATTERN[PulseCount] - 2)){
		StatusWord_Byte1 |= PatternFlag;			// Set PatternFlag HIGH;
	}
}

// Checks the Vsense and HVCurDet to determine if the output is too large or no load
void Check_Overload_noLoad(void){
	unsigned int half_Vreg = Vreg >> 2;
	P2OUT |= BIT2;									// Physically set VoltRange high
	VoltRange = HIGH;								// Set VoltRange high in software
	if (Vsense > half_Vreg){
		P2OUT |= BIT1;								// Physically set HVCurEn high
		HVCurEn = HIGH;								// Set HVCurEn high in software
		StatusWord_Byte2 &= ~OverLoadFlag;			// Set OverLoadFlag LOW;
		ILim = ILIMH;
	}
	else{
		P2OUT &= ~BIT1;								// Physically set HVCurEn low
		HVCurEn = LOW;								// Set HVCurEn low in software
	}
	if ((HVCurDet & BIT0) && (Vsense < half_Vreg)){
		P2OUT &= ~BIT2;								// Physically set VoltRange low
		VoltRange = LOW;							// Set VoltRange low in software
		P2OUT &= ~BIT1;								// Physically set HVCurEn low
		HVCurEn = LOW;								// Set HVCurEn low in software
		StatusWord_Byte2 |= OverLoadFlag;			// Set OverLoadFlag HIGH;
		ILim = ILIML;
	}
	if (CurMeas1 + CurMeas2 < A1m){
		StatusWord_Byte2 |= NoLoadFlag;				// Set NoLoadFlag HIGH;
	}
	else{
		StatusWord_Byte2 &= ~NoLoadFlag;			// Set NoLoadFlag LOW;
	}
}
