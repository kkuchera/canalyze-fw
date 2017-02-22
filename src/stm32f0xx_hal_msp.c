#include "usbd_cdc_interface.h"

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
    UNUSED(hcan);
    GPIO_InitTypeDef  GPIO_InitStruct;

    // Enable CANx and GPIO clock
    CANx_CLK_ENABLE();
    CANx_GPIO_CLK_ENABLE();

    // Configure CANx TX GPIO pin
    GPIO_InitStruct.Pin = CANx_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate =  CANx_TX_AF;

    HAL_GPIO_Init(CANx_TX_GPIO_PORT, &GPIO_InitStruct);

    // Configure CANx RX GPIO pin
    GPIO_InitStruct.Pin = CANx_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Alternate =  CANx_RX_AF;

    HAL_GPIO_Init(CANx_RX_GPIO_PORT, &GPIO_InitStruct);

    // Enable CANx RX complete interrupt
    HAL_NVIC_SetPriority(CANx_RX_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(CANx_RX_IRQn);
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan) {
    UNUSED(hcan);

    // Reset CANx
    CANx_FORCE_RESET();
    CANx_RELEASE_RESET();

    // Deinitialize CANx GPIO pins
    HAL_GPIO_DeInit(CANx_TX_GPIO_PORT, CANx_TX_PIN);
    HAL_GPIO_DeInit(CANx_RX_GPIO_PORT, CANx_RX_PIN);

    // Disable CANx RX complete interrupt
    HAL_NVIC_DisableIRQ(CANx_RX_IRQn);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim) {
    UNUSED(htim);

    // Enable TIM clock
    TIMx_CLK_ENABLE();

    // Enable TIM interrupt
    HAL_NVIC_SetPriority(TIMx_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIMx_IRQn);
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim) {
    UNUSED(htim);

    // Reset CANx
    TIMx_FORCE_RESET();
    TIMx_RELEASE_RESET();

    // Disable the TIMx interrupt
    HAL_NVIC_DisableIRQ(TIMx_IRQn);
}
