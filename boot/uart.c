#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "uart.h"


static bl_uart_recv_cb_t bl_uart_recv_cb;//定义了一个静态变量 bl_uart_recv_cb，用于存储用户注册的回调函数

void bl_uart_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2);

    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_2|GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate=115200;
    USART_InitStructure.USART_WordLength=USART_WordLength_8b;
    USART_InitStructure.USART_StopBits=USART_StopBits_1;
    USART_InitStructure.USART_Parity=USART_Parity_No;
    USART_InitStructure.USART_Mode=USART_Mode_Tx|USART_Mode_Rx;
    USART_InitStructure.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
    USART_Init(USART2,&USART_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel=USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
    NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART2,ENABLE);
}

void bl_uart_deinit(void)
{
    USART_Cmd(USART2,DISABLE);
    USART_DeInit(USART2);
}

void bl_uart_write(uint8_t *data,uint16_t size)
{
    USART_ClearFlag(USART2,USART_FLAG_TXE);
    for(int i=0;i<size;i++)
    {
        while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
        USART_SendData(USART2,data[i]);
    }
    while(USART_GetFlagStatus(USART2,USART_FLAG_TC)==RESET);
}

//用户通过调用 bl_uart_recv_register 函数来注册一个回调函数，当接收到数据时会调用这个回调函数
void bl_uart_recv_register(bl_uart_recv_cb_t callback)  
{
    bl_uart_recv_cb = callback;   //传入的回调函数指针会被存储到静态变量 bl_uart_recv_cb 中
}


void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2,USART_IT_RXNE)!=RESET)
    {
        uint8_t data=USART_ReceiveData(USART2);
        if(bl_uart_recv_cb)
        {
            bl_uart_recv_cb(&data,1); //传递长度为1字节
        }

    }
}