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
    return ticks;//��ȡ��ǰ��ϵͳʱ��
}

void Systick_Handler(void) //SysTick ��ʱ�����жϴ�����
{
    ticks++;//ticks ʼ�ռ�¼ϵͳ���������ĺ�����
}

