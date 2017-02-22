#ifndef _STM32F0xx_IT_H_
#define _STM32F0xx_IT_H_

void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USB_IRQHandler(void);
void CANx_IRQHandler(void);
void TIMx_IRQHandler(void);

#endif
