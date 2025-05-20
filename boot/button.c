#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "button.h"

void bl_button_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; // 假设按键连接在 GPIOA 的第 0 引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; // 输入模式
    GPIO_InitStructure.GPIO_PuPd= GPIO_PuPd_UP; // 上拉电阻
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure); // 初始化 GPIOA
}

bool bl_button_pressed(void)
{
    // 检测按键是否被按下
    // 假设按键按下时 GPIOA 的第 0 引脚为低电平
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == Bit_RESET;
}