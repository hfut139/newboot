#include <stdbool.h>
#include "stm32f4xx.h"
#include "led.h"

static bool led_state = false; // LED ״̬����

void bl_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin= GPIO_Pin_5; // ���� LED ������ GPIOA �ĵ� 5 ����
    GPIO_InitStructure.GPIO_Mode= GPIO_Mode_OUT; // ���ģʽ
    GPIO_InitStructure.GPIO_OType= GPIO_OType_PP; // �������
    GPIO_InitStructure.GPIO_PuPd= GPIO_PuPd_NOPULL; // ������������
    GPIO_InitStructure.GPIO_Speed= GPIO_Speed_50MHz; // 50MHz
    GPIO_Init(GPIOE, &GPIO_InitStructure); // ��ʼ�� GPIOA
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