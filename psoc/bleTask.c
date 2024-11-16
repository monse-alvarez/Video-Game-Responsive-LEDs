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
#include "bleTask.h"
#include <stdio.h>
#include "semphr.h"
#include <limits.h>

SemaphoreHandle_t bleSem;

void genericEventHandler(uint32_t event, void *eventParameter) {
    cy_stc_ble_gatts_write_cmd_req_param_t *writeReqParameter;
    // Take action based on the current event
    switch (event){
        // This event is received when the BLE stack is started
        case CY_BLE_EVT_STACK_ON:
        // Device is disconnected
        case CY_BLE_EVT_GAP_DEVICE_DISCONNECTED:
            printf("BLE not connected\r\n");
            PWM_BLINK_Start();
            Cy_BLE_GAPP_StartAdvertisement(CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
            Cy_TCPWM_TriggerReloadOrIndex(PWM_COLOR_HW, PWM_COLOR_CNT_NUM);
            Cy_TCPWM_PWM_Disable(PWM_COLOR_HW, PWM_COLOR_CNT_NUM);
        break;
        
        // Device is connected
        case CY_BLE_EVT_GATT_CONNECT_IND:
            printf("\r\nBLE Connected\r\n");
            Cy_TCPWM_TriggerReloadOrIndex(PWM_BLINK_HW, PWM_BLINK_CNT_NUM);
            Cy_TCPWM_PWM_Disable(PWM_BLINK_HW, PWM_BLINK_CNT_NUM);
            PWM_COLOR_Start();
        break;
        
        // Write request
        case CY_BLE_EVT_GATTS_WRITE_REQ:
            writeReqParameter = (cy_stc_ble_gatts_write_cmd_req_param_t *) eventParameter;
            if(CY_BLE_LED_COLOR_CHAR_HANDLE == writeReqParameter->handleValPair.attrHandle)
            {
                uint32_t val = writeReqParameter->handleValPair.value.val[0];
                if(val<=0){
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 0);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 0);
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 0);
                }
                else if(val==1){
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 0);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 1);
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 0);
                }
                else if(val==2){
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 0);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 1);
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 1);
                }
                else if(val>=3){
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 1);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 1);
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 1);
                }
                else{
                    Cy_GPIO_Write(P10_2_PORT, P10_2_NUM, 0);
                    Cy_GPIO_Write(P10_3_PORT, P10_3_NUM, 0);
                    Cy_GPIO_Write(P10_4_PORT, P10_4_NUM, 0);
                }
            }
            Cy_BLE_GATTS_WriteRsp(writeReqParameter->connHandle);
        break;
        
        default:
            break;
    }
}

/*****************************************************************************\
* Funtction: bleNotify
* Input: void (it is called inside of the ISR)
* Returns: void
* Description:
*   This is called back in the BLE ISR when an even has occured and needs to 
*   be processed. It will then set/give the semaphore to tell the BLE task to
*   process events.
\*****************************************************************************/
void bleInterruptNotify(){
    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(bleSem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*****************************************************************************\
* Funtction: bleTask
* Input: A FreeRTOS task - void * that is unused
* Returns: void
* Description:
*   This task starts the ble stack... and processes events when the bleSem
*   is set by the ISR.
\*****************************************************************************/
void bleTask(void *arg){
    (void) arg;
    printf("BLE Task Started\r\n");
    bleSem = xSemaphoreCreateCounting(UINT_MAX,0);
    Cy_BLE_Start(genericEventHandler);
    while(Cy_BLE_GetState() != CY_BLE_STATE_ON) //Get the stack going
    {
        Cy_BLE_ProcessEvents();
    }
    
    Cy_BLE_RegisterAppHostCallback(bleInterruptNotify);
   
    for(;;)
    {
        xSemaphoreTake(bleSem, portMAX_DELAY);
        Cy_BLE_ProcessEvents();
    }
}

/* [] END OF FILE */
