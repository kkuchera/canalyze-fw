#include "stm32f0xx.h"

// Wait for a specified amount of cycles
void delay(int cycles) {
    volatile int i;
    for (i=0; i < cycles; i++);
}

int main(void) {
    // Enable the AHB clock for Port B
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Set PB0 to output mode
    GPIOB->MODER |= GPIO_MODER_MODER0_0;
    
    while(1) {
        GPIOB->ODR ^= GPIO_ODR_0;
        delay(100000);
    }
}
