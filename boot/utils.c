#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "system_stm32f4xx.h"


static uint32_t ticks;

void bl_delay_init(void)
{
    SysTick_Config(SystemCoreClock/1000); // 1ms
}

void bl_delay_ms(uint32_t ms)
{
    uint32_t start=ticks;
    while(ticks-start<ms);
}

uint32_t bl_now(void)
{
    return ticks;//获取当前的系统时间
}

void Systick_Handler(void) //SysTick 定时器的中断处理函数
{
    ticks++;//ticks 始终记录系统启动以来的毫秒数
}

