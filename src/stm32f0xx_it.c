#include "stm32f0xx_it.h"
#include "stm32f0xx_hal.h"
#include "can.h" // Needed for CANx defines
#include "led.h"

extern PCD_HandleTypeDef hpcd;
extern CAN_HandleTypeDef can_handle;

/******************************************************************************
 * Processor Exception Handlers
 *****************************************************************************/

void NMI_Handler(void) {}

void HardFault_Handler(void) {
    led_on(LED_RED);
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1);
}

void SVC_Handler(void) {}

void PendSV_Handler(void) {}

void SysTick_Handler(void) {
    HAL_IncTick();
}

/******************************************************************************
 * Peripherals Interrupt Handlers
 *****************************************************************************/

void USB_IRQHandler(void) {
    HAL_PCD_IRQHandler(&hpcd);
}

void CANx_RX_IRQHandler(void) {
    HAL_CAN_IRQHandler(&can_handle);
}
