#include "led.h"
#include "stm32f0xx.h"

void led_init() {
    // Enable the AHB clock for Port B
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Set PB0 and PB1 to output mode
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER0) | GPIO_MODER_MODER0_0;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER1) | GPIO_MODER_MODER1_0;
}

void led_on(uint8_t led) {
    GPIOB->ODR |= led;
}

void led_off(uint8_t led) {
    GPIOB->ODR &= ~led;
}
