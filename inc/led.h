#ifndef _LED_H_
#define _LED_H_

#include <stdint.h>
#include "stm32f0xx_hal.h"

#define LED_GREEN   GPIO_ODR_0
#define LED_RED     GPIO_ODR_1

// Definition for TIMx clock resources
#define TIMx                            TIM3
#define TIMx_CLK_ENABLE                 __HAL_RCC_TIM3_CLK_ENABLE

#define TIMx_FORCE_RESET                __HAL_RCC_TIM3_FORCE_RESET
#define TIMx_RELEASE_RESET              __HAL_RCC_TIM3_RELEASE_RESET

// Definition for TIMx's NVIC
#define TIMx_IRQn                       TIM3_IRQn
#define TIMx_IRQHandler                 TIM3_IRQHandler

extern TIM_HandleTypeDef tim_handle;

void led_init();
void led_on(uint8_t led);
void led_off(uint8_t led);
void led_toggle(uint8_t led);
void led_blink(uint8_t led);

#endif
