/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdio.h>
#include "bleTask.h"

static SemaphoreHandle_t uartSem;

/* UART interrupt handler */
static void UART_ISR(){
    //Disable and clear the interrupt
    Cy_SCB_SetRxInterruptMask(UART_1_HW, 0);
    Cy_SCB_ClearRxInterrupt(UART_1_HW, CY_SCB_RX_INTR_NOT_EMPTY);
    NVIC_ClearPendingIRQ((IRQn_Type) UART_1_SCB_IRQ_cfg.intrSrc);
    
    // If the semaphore causes a task switch you should yield to that task
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(uartSem,&xHigherPriorityTaskWoken); // Tell the UART thread to process the RX FIFO
    if(xHigherPriorityTaskWoken != pdFALSE){
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

/* UART task */
void uartTask(void *arg)
{
    (void)arg;
    UART_1_Start();
    setvbuf( stdin, NULL, _IONBF, 0 ); // Disable STDIN input buffering
    printf("Started UART Task\r\n");
    printf("To change the leds, enter: 0 for off, 1 for red, 2 for yellow, or 3 for both leds on");

    /* Create a semaphore. It will be set in the UART ISR when data is available */
    uartSem = xSemaphoreCreateBinary();    
 
    /* Configure the interrupt and interrupt handler for the UART */
    (void) Cy_SysInt_Init(&UART_1_SCB_IRQ_cfg, &UART_ISR); 
    NVIC_EnableIRQ((IRQn_Type) UART_1_SCB_IRQ_cfg.intrSrc);
    Cy_SCB_SetRxInterruptMask(UART_1_HW,CY_SCB_RX_INTR_NOT_EMPTY);
    
    while(1)
    {
        /* Wait here until the semaphore is given (i.e. set) by the ISR */
        xSemaphoreTake(uartSem,portMAX_DELAY);

        /* Process all of hte characters in the command buffer */
        while(Cy_SCB_UART_GetNumInRxFifo(UART_1_HW))
        {
            char c= getchar();
            switch(c)
            {
                case '?':
                    printf("%c\r\n", c);
                    printf("To change the leds, enter: 0 for off, 1 for red, 2 for yellow, or 3 for all leds on\r\n");
                break;
                
                case '0':
                    printf("%c\r\n", c);
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 0);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 0);
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 0);
                    printf("LEDS OFF\r\n");
                break;
                 
                case '1':
                    printf("%c\r\n", c);
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 0);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 1);
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 0);
                    printf("RED LED ON\r\n");
                break;
                    
                case '2':
                    printf("%c\r\n", c);
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 0);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 1);    
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 1);  
                    printf("YELLOW LED ON\r\n");
                break;
                    
                case '3':
                    printf("%c\r\n", c);
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 1);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 1);    
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 1);  
                    printf("GREEN LED ON\r\n");
                break;
                    
                default:
                    printf("To change the leds, enter: 0 for off, 1 for red, 2 for yellow, 3 for green, or 4 for all leds on\r\n");
                break;
            }
        }
        // Once you have processed the RX FIFO reenable the interrupt
        Cy_SCB_SetRxInterruptMask(UART_1_HW,CY_SCB_RX_INTR_NOT_EMPTY);
    }
}

/* [] END OF FILE */
