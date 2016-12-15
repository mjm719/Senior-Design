//******************************************************************************
//	--Mike McConville--
//
//	This file is the to be used for a Master MSP430 talking to a slave MSP430
//	via I2C in both write and read modes. This file is the master portion of
//	the code. This code is not complete for the task of Bill's project, and
//  was more or less just wrote to test the slave side of the project. Some
//  features have been added that will be used in the actual master code though.
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

// ADC Variables reported back from Slave
unsigned int IgfiMeas;
unsigned int CurMeas1;
unsigned int CurMeas2;
unsigned int Vsense;
unsigned int Vreg;
unsigned int VOvp;

// Status Variables reported back from Slave
unsigned char StatusWord_Byte2;
unsigned char StatusWord_Byte1;
unsigned char IOByte;

// Variables to send to Slave
unsigned int TempOvp;
unsigned int TempV;

// Variables used in I2C for Addressing Slave Microprocessors
unsigned char TXData = 0x00;
unsigned char TXByteCtr;
unsigned char I2C_Action;
unsigned int OperationComplete = FALSE;
unsigned char RXByteCtr;
unsigned char RxReadings[12];
unsigned char *PRxReadings;                     // Pointer to RXReadings
unsigned char RxStatus[12];
unsigned char *PRxStatus;                     // Pointer to RXStatus
unsigned char I2C_Action_toSend;
unsigned char Slave_to_Address;

// Variables used in I2C for Addressing Potentiometers
unsigned char Talking_to_Pot;
unsigned char volWrite = 0;
unsigned char Pot_to_Address;

// Variables to wait for while loop exit
long num_to_delay = 50000;
unsigned int delay_count = 0;
unsigned int wait_operation_count = 0;

// Function Prototypes
void Setup_TX(void);
void Setup_RX(void);
void Setup_Write_to_Pots(void);
void Write_to_Pot(void);
void wait_for_stop(void);
void wait_for_start(void);
void wait_for_operation_completion(void);

// Function Prototypes for I2C Data Transmissions
void Send_I2C_Action(void);
void Transfer_New_Voltage_Variables(void);
void Receive_Status_Variables(void);
void Receive_Readings(void);

// Function Prototypes for turning I2C data into variables
void Get_Status_Variables(void);
void Get_Readings_Variables(void);

//******* MAIN FUNCTION *******//

int main(void)
{

 	WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
	BCSCTL1 = CALBC1_8MHZ;
	DCOCTL = CALDCO_8MHZ;

	P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	TXData = 0x00;                            // Holds TX data
	UCB0I2CIE = UCNACKIE;

	// If you want to use volatile memory on the potentiometers, do an initial  write to each
	// potentiometer with volWrite set to 1. To accomplish this, set the address of the potentiometer,
	// then set volWrite = 1,  and thencall the function Write_to_Pot();


	volWrite = 1;

	// This is an example of how to set up volatile memory. You must do this for each potentiometer
	Pot_to_Address = 0x52;
	Write_to_Pot();

	volWrite = 0;

	while (1)
	{
		// Before you start using the potentiometers to actually change the output voltage, you need to set them to use volatile memory
		// because volatile memory is much quicker

		Slave_to_Address = 0x48;

		// In order to transmit or receive data, call the appropriate function for your desired
		// I2C data transmission
		// You will need to also change the Slave_to_Address variable

		// If using the Send_I2C_Action, set I2C_Action_toSend to the transaction you will next complete
		// to the value of the next action you wish to complete and then call Send_I2C_Action.

		// Send your next I2C_Action, which is transfer new voltage settings variables
		I2C_Action_toSend = 1;
		Send_I2C_Action();

		// Transfer your new voltage settings variables
		Transfer_New_Voltage_Variables();

		// Send your next I2C_Action, which is Read Status Variables in this case
		I2C_Action_toSend = 2;
		Send_I2C_Action();

		// Receive the StatusWord and IO Byte Variables
		Receive_Status_Variables();
		Get_Status_Variables();

	    // Send your next I2C_Action, which is read Voltages and Currents in this case
		I2C_Action_toSend = 3;
		Send_I2C_Action();

		// Receive Voltages and Currents
		Receive_Readings();
		Get_Readings_Variables();

	    //Write Data to Potentiometers
		Write_to_Pot();


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
	// I2C for addressing slave microprocessors
	if (Talking_to_Pot == 0){
		// Send I2C_Action Command
		if (I2C_Action == 0){
			if (TXByteCtr){                        		// Check TX byte counter
				UCB0TXBUF = TXData;                     // Load TX buffer
				TXByteCtr--;                            // Decrement TX byte counter
			}
			else{
				UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
				IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag
				OperationComplete = TRUE;
			}
		}

		// Send the New Voltage Variables
		else if (I2C_Action == 1){
			if (TXByteCtr){                        		// Check TX byte counter
				if (TXByteCtr == 1){
					UCB0TXBUF = (TempOvp >> 8) & 0xFF;                     // Send the MSB of TempOvp
					TXByteCtr--;
				}
				else if (TXByteCtr == 2){
					UCB0TXBUF = (TempOvp & 0xFF);
					TXByteCtr--;                          // Send the LSB of TempOvp
				}
				else if (TXByteCtr == 3){
					UCB0TXBUF = (TempV >> 8) & 0xFF;
					TXByteCtr--;                          // Send the MSB of TempV
				}
				else if (TXByteCtr == 4){
					UCB0TXBUF = (TempV & 0xFF);
					TXByteCtr--;                          // Send the LSB of TempV
				}
				else if (TXByteCtr == 5){
					UCB0TXBUF = TempDelta;
					TXByteCtr--;                          // Send TempDelta
				}
			}
			else{
				UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
				IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag
				OperationComplete = TRUE;
			}
		}
		// Receive Status and IO Bytes from the slave microprocessor
		else if (I2C_Action == 2){
			RXByteCtr--;
			if (RXByteCtr){
				*PRxStatus++ = UCB0RXBUF;                 // Move RX data to address PRxStatus
				if (RXByteCtr == 1){                     // Only one byte left?
				  UCB0CTL1 |= UCTXSTP;                  // Generate I2C stop condition
				}
			}
			else{
				*PRxStatus = UCB0RXBUF;                   // Move final RX data to PRxStatus
				OperationComplete = TRUE;
			}
		}
		// Receive the ADC voltage and current readings from the slave microprocessor
		else if (I2C_Action == 3){
			RXByteCtr--;
			if (RXByteCtr){
				*PRxReadings++ = UCB0RXBUF;                 // Move RX data to address PRxReadings
				if (RXByteCtr == 1){                     // Only one byte left?
				  UCB0CTL1 |= UCTXSTP;                  // Generate I2C stop condition
				}
			}
			else{
				*PRxReadings = UCB0RXBUF;                   // Move final RX data to PRxReadings
				OperationComplete = TRUE;
			}
		}
	}

	// I2C for Addressing Potentiometers
	else if (Talking_to_Pot == 1){
		if (volWrite == 1){
			if (TXByteCtr == 1){
				UCB0TXBUF = 0xC0;                     // Load TX buffer
				IFG2 |= UCA0TXIFG;
				TXByteCtr--;
			}
			else if (TXByteCtr == 2){
				UCB0TXBUF = 0x10;
				IFG2 |= UCA0TXIFG;
				TXByteCtr--;
			}
		}
		else if (TXByteCtr){                        	// Check TX byte counter
			if (TXByteCtr == 1){
				UCB0TXBUF = 0x54;                     // Load TX buffer with actual data to be sent
				IFG2 |= UCA0TXIFG;
				TXByteCtr--;                            // Decrement TX byte counter
			}
			else if (TXByteCtr == 2){
				UCB0TXBUF = 0x00;
				IFG2 |= UCA0TXIFG;
				TXByteCtr--;
			}
		}
		else{
			UCB0CTL1 |= UCTXSTP;                    // I2C stop condition
			IFG2 &= ~UCB0TXIFG;                     // Clear USCI_B0 TX int flag
			OperationComplete = TRUE;
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
	if (UCB0STAT & UCNACKIFG){
		UCB0CTL1 |= UCTXSTP;
		UCB0STAT &= ~UCNACKIFG;
		OperationComplete = TRUE;
	}
}

// This function prepares the unit to transfer data to a slave microprocessor. The exact unit it addresses
// is determined by the Slave_to_Address variable and must be set before the function is called
void Setup_TX(void) {
	__disable_interrupt();
	Talking_to_Pot = 0;
	IE2 &= ~UCB0RXIE;
	IFG2 &= ~UCA0TXIFG;
	wait_for_stop();
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 80;                             // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = Slave_to_Address;             // Slave Address is 048h
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	IE2 |= UCB0TXIE;                          // Enable TX interrupt
}

// This function prepares the unit to receive data to a slave microprocessor. The exact unit it receives from
// is determined by the Slave_to_Address variable and must be set before the function is called
void Setup_RX(void){
	__disable_interrupt();
	Talking_to_Pot = 0;
	IE2 &= ~UCB0TXIE;
	IFG2 &= ~UCA0RXIFG;
	wait_for_stop();
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 80;                             // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = Slave_to_Address;             // Slave Address is 048h
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	IE2 |= UCB0RXIE;                          // Enable RX interrupt
}

// This function prepares the unit to transfer data to a potentiometer. The exact unit it addresses
// is determined by the Pot_to_Address variable and must be set before the function is called
void Setup_Write_to_Pots(void){
	__disable_interrupt();
	Talking_to_Pot = 1;						  //Set the transaction to address to the potentiometers
	IE2 &= ~UCB0RXIE;
	IFG2 &= ~UCA0TXIFG;
	wait_for_stop();
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
	UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
	UCB0BR0 = 80;                             // fSCL = SMCLK/12 = ~100kHz
	UCB0BR1 = 0;
	UCB0I2CSA = Pot_to_Address;               // Slave Address is 048h
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	IE2 |= UCB0TXIE;                          // Enable TX interrupt
}

// This function call the setup write to pots and then transfers the data over I2C to the potentiometer
void Write_to_Pot(void){
    Setup_Write_to_Pots();
	TXByteCtr = 2;                          // Load TX byte counter
	wait_for_stop();
	UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
	wait_for_operation_completion();
}

// This function is used as a escape to a stop while loop after a set amount of time. The amount of time waited
// depends on the value of num_to_delay. With num_to_delay, the approximate wait time is 35ms
void wait_for_stop(void){
	delay_count = 0;
	while (UCB0CTL1 & UCTXSTP){               // Ensure stop condition got sent
    	if (delay_count < num_to_delay){
    		delay_count++;
    		__delay_cycles(4);
    	}
    	else{
    		UCB0CTL1 |= UCTXNACK;
    		UCB0CTL1 |= UCSWRST;                      // Enable SW reset
    		break;
    	}
	}
}

// This function is used as a escape to a start while loop after a set amount of time. The amount of time waited
// depends on the value of num_to_delay. With num_to_delay, the approximate wait time is 35ms
void wait_for_start(void){
    if (UCB0CTL1 & UCTXSTT){
    	delay_count = 0;
		while (UCB0CTL1 & UCTXSTT){             // Start condition sent?
	    	if (delay_count < num_to_delay){
	    		delay_count++;
	    	}
	    	else {
	    		UCB0CTL1 |= UCTXNACK;
	    		UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	    		break;
	    	}
		}
    }
}

// This function is used as a escape to a wait for operation completion while loop after a set amount
// of time. The amount of time waited depends on the value of num_to_delay. With num_to_delay,
// the approximate wait time is 35ms.
// This function is also responsible for enabling the interrupts that start I2C communication and works by
// jumping out of the while loop when the ISR hits the end of its function setting OperationComplete to TRUE
void wait_for_operation_completion(void){
	__bis_SR_register(GIE);        // Enable interrupts
	wait_operation_count = 0;
	while (OperationComplete == FALSE){
		wait_operation_count++;
		if (wait_operation_count > num_to_delay){
			UCB0CTL1 |= UCTXNACK;
			UCB0CTL1 |= UCSWRST;                      // Enable SW reset
			break;
		}
	}
	OperationComplete = FALSE;
}

// This function sends the next I2C_Action the slave microprocessor is supposed to complete
void Send_I2C_Action(void){
	Setup_TX();
	I2C_Action = 0;
	TXData = I2C_Action_toSend;
	TXByteCtr = 1;    						// Load TX byte counter
	wait_for_stop();
	UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
	wait_for_operation_completion();
}

// This function sets the master up to transfer the new variables the slave microcprocessor should use
void Transfer_New_Voltage_Variables(void){
	Setup_TX();
	I2C_Action = 1;
	TXByteCtr = 5;    						// Load TX byte counter
	wait_for_stop();
	UCB0CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
	wait_for_operation_completion();
}

// This function sets the master up to receive 3 data bytes that contain StatusWord_Byte2, StatusWord_Byte1
// and IOByte
void Receive_Status_Variables(void){
	Setup_RX();
	PRxStatus = (unsigned char *)RxStatus;    // Start of RX buffer
	I2C_Action = 2;
	RXByteCtr = 3;
	wait_for_stop();
    UCB0CTL1 |= UCTXSTT;                    // I2C start condition
    wait_for_start();
    wait_for_operation_completion();
}

// This function sets the master up to receive 12 data bytes that are the ADC measurements of the slave
// device on the output rails.
void Receive_Readings(void){
	Setup_RX();
    PRxReadings = (unsigned char *)RxReadings;    // Start of RX buffer
	I2C_Action = 3;
	RXByteCtr = 12;
	wait_for_stop();
	UCB0CTL1 |= UCTXSTT;                    // I2C start condition
	wait_for_start();
	wait_for_operation_completion();
}

// This function takes the data that has been received and stored in RxStatus array and makes usuable variables
// out of the elements of the array
void Get_Status_Variables(void){
	StatusWord_Byte2 = RxStatus[0];
	StatusWord_Byte1 = RxStatus[1];
	IOByte = RxStatus[2];
}

// This function takes the data that has been received and stored in RxReadings array and makes usuable variables
// out of the elements of the array
void Get_Readings_Variables(void){
	IgfiMeas = (RxReadings[0] << 8) | RxReadings[1];
	CurMeas1 = (RxReadings[2] << 8) | RxReadings[3];
	CurMeas2 = (RxReadings[4] << 8) | RxReadings[5];
	Vsense = (RxReadings[6] << 8) | RxReadings[7];
	Vreg = (RxReadings[8] << 8) | RxReadings[9];
	VOvp = (RxReadings[10] << 8) | RxReadings[11];
}
