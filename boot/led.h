#ifndef __LED_H
#define __LED_H
#include <stdbool.h>
#include "stm32f4xx.h"

void bl_led_init(void);
void bl_led_set(bool on);
void bl_led_on(void);
void bl_led_off(void);
void bl_led_toggle(void);

#endif