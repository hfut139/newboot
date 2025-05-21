#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "stm32f4xx.h"
#include "button.h"

void bl_button_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; // ���谴�������� GPIOA �ĵ� 0 ����
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN; // ����ģʽ
    GPIO_InitStructure.GPIO_PuPd= GPIO_PuPd_UP; // ��������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure); // ��ʼ�� GPIOA
}

bool bl_button_pressed(void)
{
    // ��ⰴ���Ƿ񱻰���
    // ���谴������ʱ GPIOA �ĵ� 0 ����Ϊ�͵�ƽ
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == Bit_RESET;
}