/**
 * @file main.c
 *
 * Main program
 *
 * The general structure of the program is that interrupt handlers set a flag
 * @see requests.h indicating a request. The main program then checks what has
 * been requested and handles it. For example, when a command to open CAN is
 * received over USB, the interrupt handler sets a flag REQ_CAN_OPEN indicating
 * that the CAN should be started. The main loop then checks these flags, sees
 * a request that CAN should be opened, and opens CAN. There are multiple
 * advantages to this approach.
 *  1. Keeps interrupt service routines short.
 *  2. Clarity since the workings of the whole program are in the main loop as
 *  opposed to being scattered around different interrupt service routines.
 *  3. The main thread schedules when to handle a request instead of each ISR
 *  living a life of its own.
 * Aside from these advantages, there really isn't any other choice since HAL
 * functions can only be called from the main thread since HAL can be locked
 * otherwise.
 *
 * Since FS USB (12Mbit/s) is a lot faster than CAN (max 1Mbit/s), data can be
 * sent to the host over USB at any time. Data can only be received over USB
 * when the previous packet has been handled. This is to prevent the device
 * from being flooded with incoming data. In addition, this guarantees that CAN
 * and USB data won't be corrupted since it will only receive new data once the
 * previous data has been handled.
 */
#include "can.h"
#include "led.h"
#include "requests.h"
#include "stm32f0xx.h"
#include "usbd.h"
#include "usbd_8dev_if.h"

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

/**
 * Monitors CAN traffic and relays it to USB interface and vise versa.
 *
 * @see usbd_8dev_if.c for more details on how this works.
 */
int main(void) {
    HAL_Init();
    clock_init();
    usb_init();
    can_init();
    led_init();

    led_on(LED_GREEN);

    requests = 0;
    //TODO 1) make all calls on main thread non blocking, 2) handle CAN errors
    while (1) {
        if (requests & REQ_VER) {
            usbd_8dev_send_cmd_rsp(0);
            requests &= ~REQ_VER;
        }
        if (requests & REQ_CAN_OPEN) {
            usbd_8dev_send_cmd_rsp(can_open());
            requests &= ~REQ_CAN_OPEN;
        }
        if (requests & REQ_CAN_CLOSE) {
            usbd_8dev_send_cmd_rsp(can_close());
            requests &= ~REQ_CAN_CLOSE;
        }
        if (requests & REQ_CAN_TX) {
            if (can_tx(10)) {
                // Will occur when there is a timeout or there are still
                // pending messages from last attempt
                // TODO Maybe add buffer or cancel pending when timeout or
                // error occurs?
                //led_on(LED_RED);
            }
            usbd_8dev_receive();
            requests &= ~REQ_CAN_TX;
        }
        if (requests & REQ_CAN_ERR) {
            usbd_8dev_transmit_can_error();
            requests &= ~REQ_CAN_ERR;
        }
        if (can_msg_pending()) {
            if (can_rx(3)) {
                led_on(LED_RED);
            } else {
                usbd_8dev_transmit_can_frame();
            }
        }
    }
}
