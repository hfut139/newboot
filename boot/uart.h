#ifndef __UART_H
#define __UART_H

#include <stdint.h>

typedef void (*bl_uart_recv_cb_t)(uint8_t *data, uint32_t len);//定义了一个函数指针类型 bl_uart_recv_cb_t，用于表示接收数据的回调函数。


void bl_uart_init(void);
void bl_uart_deinit(void);
void bl_uart_write(uint8_t *data, uint32_t len);
void bl_uart_recv_register(bl_uart_recv_cb_t callback);


#endif