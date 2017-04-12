#ifndef _CAN_H_
#define _CAN_H_

#include <stdint.h>
#include "stm32f0xx_hal.h"

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


/* Note: if the synchronization jump width (sjw), bit segment 1 (bs1) or bit
 * segment 2 (bs2) received from device driver is equal to x, the value x-1 is
 * stored in sjw, ts1 and ts2 respectively. This allows these variables to be
 * stored in 1 byte.
 */
typedef struct can_bittiming {
    uint8_t sjw;    /* Synchronization jump width (in tq) (0-3) */
    uint8_t ts1;    /* Bit segment 1 (in tq) (0-15) */
    uint8_t ts2;    /* Bit segment 2 (in tq) (0-7) */
    uint16_t brp;   /* Bit-rate prescaler */
} Can_BitTimingTypeDef;

extern CAN_HandleTypeDef can_handle;

uint8_t can_init();
void can_open_req(Can_BitTimingTypeDef* can_bittiming, uint8_t ctrlmode);
uint8_t can_open();
uint8_t can_close();
uint8_t can_tx();
uint8_t can_rx();
uint8_t can_msg_pending();

#endif
