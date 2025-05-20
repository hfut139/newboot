#include <stdbool.h>
#include "stm32f4xx.h"
#include "led.h"

static bool led_state = false; // LED 状态变量

void bl_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin= GPIO_Pin_5; // 假设 LED 连接在 GPIOA 的第 5 引脚
    GPIO_InitStructure.GPIO_Mode= GPIO_Mode_OUT; // 输出模式
    GPIO_InitStructure.GPIO_OType= GPIO_OType_PP; // 推挽输出
    GPIO_InitStructure.GPIO_PuPd= GPIO_PuPd_NOPULL; // 无上下拉电阻
    GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz; // 50MHz
    GPIO_Init(GPIOE, &GPIO_InitStructure); // 初始化 GPIOA
    bl_led_off(); 
}

void bl_led_set(bool on)
{
    led_state=on;
    GPIO_WriteBit(GPIOE,GPIO_Pin_5,on? Bit_RESET:Bit_SET);

}

void bl_led_on(void)
{
    led_state=true;
    bl_led_set(true);
}

void bl_led_off(void)
{
    led_state=false;
    bl_led_set(false);
}

void bl_led_toggle(void)
{
    led_state=!led_state;
    bl_led_set(led_state);
}