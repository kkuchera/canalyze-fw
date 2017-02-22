#ifndef _USBD_CDC_IF_H_
#define _USBD_CDC_IF_H_

#include "usbd_cdc.h"

// Definition for CANx clock resources
#define CANx                            CAN
#define CANx_CLK_ENABLE()               __HAL_RCC_CAN1_CLK_ENABLE()
#define CANx_GPIO_CLK_ENABLE()          __HAL_RCC_GPIOB_CLK_ENABLE()

#define CANx_FORCE_RESET()              __HAL_RCC_CAN1_FORCE_RESET()
#define CANx_RELEASE_RESET()            __HAL_RCC_CAN1_RELEASE_RESET()

// Definition for CANx Pins 
#define CANx_TX_PIN                     GPIO_PIN_9
#define CANx_TX_GPIO_PORT               GPIOB  
#define CANx_TX_AF                      GPIO_AF4_CAN
#define CANx_RX_PIN                     GPIO_PIN_8
#define CANx_RX_GPIO_PORT               GPIOB 
#define CANx_RX_AF                      GPIO_AF4_CAN

// Definition for CANx's NVIC
#define CANx_RX_IRQn                    CEC_CAN_IRQn
#define CANx_RX_IRQHandler              CEC_CAN_IRQHandler

// Definition for TIMx clock resources
#define TIMx                             TIM3
#define TIMx_CLK_ENABLE()               __HAL_RCC_TIM3_CLK_ENABLE()
#define TIMx_FORCE_RESET()              __HAL_RCC_TIM3_FORCE_RESET()
#define TIMx_RELEASE_RESET()            __HAL_RCC_TIM3_RELEASE_RESET()

// Definition for TIMx's NVIC
#define TIMx_IRQn                       TIM3_IRQn
#define TIMx_IRQHandler                 TIM3_IRQHandler

// Period (in ms) for transmitting buf_tx. Max is 65, min is 1
#define CDC_POLLING_INTERVAL            5 

extern USBD_CDC_ItfTypeDef  USBD_CDC_fops;

#endif
