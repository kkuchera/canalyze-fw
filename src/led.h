#ifndef _LED_H_
#define _LED_H_

#include <stdint.h>

#define LED_GREEN   GPIO_ODR_0
#define LED_RED     GPIO_ODR_1

void led_init();
void led_on(uint8_t led);
void led_off(uint8_t led);

#endif
