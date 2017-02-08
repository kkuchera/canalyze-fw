#include "led.h"
#include "stm32f0xx.h"

// Wait for a specified amount of cycles
void delay(int cycles) {
    volatile int i;
    for (i=0; i < cycles; i++);
}

void clock_init() {
    // TODO For robust implementation, add time-out management in while loops
    // Derived from p940 A.3.2 PLL configuration modification code example

    // CSS automatically disables HSE (and PLL) and switches to HSI if failure
    // is detected
    // Turn on external crystal
    RCC->CR |= RCC_CR_CSSON | RCC_CR_HSEON;
    // Wait for crystal to be stable
    while ((RCC->CR & RCC_CR_HSERDY) == 0);

    // Test if PLL is used as System clock
    if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL) {
        // Select HSI as system clock
        RCC->CFGR &= (uint32_t) (~RCC_CFGR_SW);
        // Wait for HSI switched
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);
    }
    // Disable the PLL
    RCC->CR &= (uint32_t) (~RCC_CR_PLLON);
    // Wait until PLLRDY is cleared
    while((RCC->CR & RCC_CR_PLLRDY) != 0);
    // Change the desired parameters
    // Set the PLL multiplier to 3 (48Mhz)
    RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_PLLMUL)) | (RCC_CFGR_PLLMUL3);
    // Set the PLL divider (prediv) to 1
    RCC->CFGR2 = (RCC->CFGR2 & (~RCC_CFGR2_PREDIV)) | (RCC_CFGR2_PREDIV_DIV1);
    // Set PLL source to HSE
    RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_PLLSRC)) | (RCC_CFGR_PLLSRC_HSE_PREDIV);
    // Enable the PLL
    RCC->CR |= RCC_CR_PLLON;
    // Wait until PLLRDY is set
    while((RCC->CR & RCC_CR_PLLRDY) == 0);
    // Select PLL as system clock
    RCC->CFGR |= (uint32_t) (RCC_CFGR_SW_PLL);
    // Wait until the PLL is switched on
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

int main(void) {
    clock_init();
    led_init();

    while(1) {
        led_on(LED_GREEN);
        delay(1000000);
        led_off(LED_GREEN);
        led_on(LED_RED);
        delay(1000000);
        led_off(LED_RED);
    }
}
