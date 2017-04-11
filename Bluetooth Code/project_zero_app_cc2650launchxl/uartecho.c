/*
 * uartecho.c
 *
 *  Created on: Apr 7, 2017
 *      Author: ap58677
 */


/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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
 */

/*
 *  ======== uartecho.c ========
 */


#include <ti/drivers/UART.h>
//#include <uart_logs.h>
/* Example/Board Header files */
#include "Board.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "data_service.h"
/* BIOS Header files */
#include <ti/sysbios/knl/Task.h>

#define TASKSTACKSIZE     768

Task_Struct task0Struct;
Char task0Stack[TASKSTACKSIZE];




/*extern void UART_Read(void)
{
    char input[3] = {0};
    //char output[3] = {0};
    int num_bytes =0;
    //int inc = 0;
    int len = (sizeof(input)/sizeof(char));
    UART_Handle uart;
    UART_Params uartParams;

    /* Create a UART with data processing off.
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    //uartParams.readMode = UART_MODE_CALLBACK; //
    //uartParams.writeMode =UART_MODE_CALLBACK; //
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 9600;
    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL)
    {
        ;//System_abort("Error opening the UART");
    }

    /* Loop forever echoing
    /*while (1)
    {
        memset(input,0,sizeof(input));
        __delay_cycles(1000000);
   //   if(num_bytes= UART_write(uart, received_string, len))
                {
                    if(num_bytes == UART_ERROR)
                    {
                        printf("Error writing to MSP430\n");
                    }

                }
        if(num_bytes= UART_read(uart, &input, len))
        {
            if(num_bytes == UART_ERROR)
            {
                printf("Error reading from MSP430\n");
            }

        }
        // Initalization of characteristics in Data_Service that can provide data.
        DataService_SetParameter(DS_STRING_ID, sizeof(input), input);

    //}
}*/
extern void UART_MCU(uint8_t *write_string)
{
    char input[3] = {0};
        //char output[3] = {0};
        //int num_bytes =0;
        //int inc = 0;
    int len_read = (sizeof(input)/sizeof(char));
    int num_bytes = 0;
    //int len = (sizeof(received_string)/sizeof(uint8_t));
    UART_Handle uart;
    UART_Params uartParams;
    //Create a UART with data processing off.
       UART_Params_init(&uartParams);
       uartParams.writeDataMode = UART_DATA_BINARY;
       uartParams.readDataMode = UART_DATA_BINARY;
       uartParams.readReturnMode = UART_RETURN_FULL;
       //uartParams.readMode = UART_MODE_CALLBACK;
       uartParams.readEcho = UART_ECHO_OFF;
       uartParams.baudRate = 9600;
       uart = UART_open(Board_UART0, &uartParams);
    //uint8_t *write_string;
    while(1)
    {
       //write_string = received_string;
       if (uart == NULL)
       {
           ;//System_abort("Error opening the UART");
       }
           memset(input,0,sizeof(input));
           __delay_cycles(1000000);
            if(num_bytes= UART_read(uart, &input, len_read))
                   {
                       if(num_bytes == UART_ERROR)
                       {
                           printf("Error reading from MSP430\n");
                       }
                       else
                       {
                         // Initalization of characteristics in Data_Service that can provide data.
                           DataService_SetParameter(DS_STRING_ID, sizeof(input), input);
                       }

                   }
            if(num_bytes= UART_write(uart,write_string, 3))
                {
                    if(num_bytes == UART_ERROR)
                        {
                           printf("Error writing to MSP430\n");
                        }
                    //DataService_SetParameter(DS_STREAM_ID, DS_STREAM_LEN, initVal);
                }
            memset(input,0,sizeof(input));
       }
}

/*extern void UART_Start(uint8_t *received_string)
{
    Task_Params taskParams;
    UART_init();
    // Construct BIOS objects
    Task_Params_init(&taskParams);
         taskParams.stackSize = TASKSTACKSIZE;
         taskParams.stack = &task0Stack;
         Task_construct(&task0Struct, (Task_FuncPtr)UART_MCU, &taskParams, NULL);
}*/

/*extern void UART_Write(uint8_t *received_string)
{
    Task_Params taskParams;
    UART_init();
    // Construct BIOS objects
    Task_Params_init(&taskParams);
         taskParams.stackSize = TASKSTACKSIZE;
         taskParams.stack = &task0Stack;
         Task_construct(&task0Struct, (Task_FuncPtr)UART_MCU, &taskParams, NULL);
}*/

