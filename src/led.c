#include "led.h"
#include "stm32f0xx.h"

#define BLINK_PERIOD    500  /*< Blink period in ms. */

TIM_HandleTypeDef tim_handle;

static volatile uint8_t blink; /*< Led that is blinking. */

static void tim_config();
static void tim_start();
static void tim_stop();
static void error_handler();

void led_init() {
    // Enable the AHB clock for Port B
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Set PB0 and PB1 to output mode
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER0) | GPIO_MODER_MODER0_0;
    GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODER1) | GPIO_MODER_MODER1_0;

    blink = 0;
}

void led_on(uint8_t led) {
    if (blink == led) tim_stop();
    GPIOB->ODR |= led;
}

void led_off(uint8_t led) {
    if (blink == led) tim_stop();
    GPIOB->ODR &= ~led;
}

void led_toggle(uint8_t led) {
    GPIOB->ODR ^= led;
}

void led_blink(uint8_t led) {
    if (blink) tim_stop();
    blink = led;
    tim_start();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    UNUSED(htim);
    led_toggle(blink);
}

void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim) {
    UNUSED(htim);
    error_handler();
}

static void tim_start() {
    tim_config();
    if (HAL_TIM_Base_Start_IT(&tim_handle)) {
        error_handler();
    }
}

static void tim_stop() {
    if (HAL_TIM_Base_DeInit(&tim_handle)) {
        error_handler();
    }
    blink = 0;
}

static void tim_config() {
    /* Set TIMx instance */
    tim_handle.Instance = TIMx;

    tim_handle.Init.Period = BLINK_PERIOD - 1;
    tim_handle.Init.Prescaler = 48000 - 1;
    tim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if(HAL_TIM_Base_Init(&tim_handle)) {
        error_handler();
    }
}

static void error_handler() {
    led_on(LED_RED);
}
