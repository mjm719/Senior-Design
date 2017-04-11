/*
 * uartecho.h
 *
 *  Created on: Apr 7, 2017
 *      Author: ap58677
 */

#ifndef UARTECHO_H_
#define UARTECHO_H_

extern void UART_Write(uint8_t *received_string);
extern void UART_MCU(uint8_t *received_string);
//extern uint8_t * volatile write_string;
//extern void UART_MCU(uint8_t *received_string);
//extern void UART_Read(void);

#endif /* UARTECHO_H_ */
