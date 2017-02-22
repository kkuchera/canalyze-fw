#include "led.h"
#include "stm32f0xx.h"
#include "usbd.h"
#include "usbd_cdc_interface.h"

extern CAN_HandleTypeDef can_handle;


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
        RCC->CFGR &= ~RCC_CFGR_SW;
        // Wait for HSI switched
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);
    }
    // Disable the PLL
    RCC->CR &= ~RCC_CR_PLLON;
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
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    // Wait until the PLL is switched on
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    // Set PCLK (APB) to SYSCLK/1
    RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_PPRE)) | (RCC_CFGR_PPRE_DIV1);
    // Set HCLK (AHB) to SYSCLK/1
    RCC->CFGR = (RCC->CFGR & (~RCC_CFGR_HPRE)) | (RCC_CFGR_HPRE_DIV1);
    // Set the USB clock source to PLL
    RCC->CFGR3 = (RCC->CFGR3 & (~RCC_CFGR3_USBSW)) | (RCC_CFGR3_USBSW_PLLCLK);

    // Set systick to 1ms (HCLK = PLL = 48MHz), done in HAL_Init();
    //SysTick_Config(48000000/1000);
    //NVIC_SetPriority(SysTick_IRQn, 0);
}

int main(void) {
    HAL_Init();
    clock_init();
    led_init();
    usb_init();

    led_on(LED_GREEN);

    while(1) {
    } 
}
