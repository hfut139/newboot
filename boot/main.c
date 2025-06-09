#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "uart.h"
#include "button.h"
#include "led.h"

#define LOG_TAG "main"
#define LOG_LVL ELOG_LVL_INFO
#include "elog.h"

extern void bl_lowlevel_init(void);
extern void bootloader_main(uint32_t boot_delay);
extern bool verify_application(void);


static bool button_trap_boot(void)
{
    if (bl_button_pressed())
    {
        bl_delay_ms(100);
        return bl_button_pressed();
    }

    return false;
}

static void button_wait_release(void)
{
    while (bl_button_pressed())
    {
        bl_delay_ms(100);
    }
}

int main(void)
{
    bl_lowlevel_init();

#if DEBUG
    elog_init();
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_TAG);
    elog_start();
#endif

    bl_delay_init();
    bl_led_init();
    bl_button_init();
    bl_uart_init();

    log_d("button: %d", bl_button_pressed());

    bool trap_boot=false;
    
    if(button_trap_boot())
    {
        log_w("button pressed,trap into boot");
        trap_boot=true;  //强制进入boot模式
    }

    else if (!verify_application())
    {
        log_w("application verify failed,trap into boot");
        trap_boot=true;  //应用程序校验失败，强制进入boot模式
    }

    if(trap_boot)
    {
        //提醒用户已经进入boot模式，可以松开按钮
        bl_led_on();
        button_wait_release();
    }

    // trap_boot == 0: Boot升级模式：按键按下||应用程序校验失败,永远停留在Boot模式,等待上位机升级
    // trap_boot == 3: 正常启动模式：按键未按下&&应用程序校验通过，3秒后自动启动应用程序
    bootloader_main(trap_boot ? 0 : 3);

    
    return 0;
}

// 启动 → main.c
//   ↓
// 硬件初始化 → lowlevel.c, utils.c, button.c, uart.c
//   ↓
// 模式判断 → 按键检测 + 应用程序校验(arginfo.c)
//   ↓
// bootloader_main() → bootloader.c
//   ↓
// ┌─ 正常模式: 3秒后 boot_application()
// │
// └─ Boot模式: 等待上位机命令
//     ↓
//   串口中断 → uart.c (USART2_IRQHandler)
//     ↓
//   环形缓冲区 → serial_recv_callback()
//     ↓
//   状态机解析 → bl_recv_handler()
//     ↓
//   命令处理 → bl_pkt_handler()
//     ↓
//   ┌─ ERASE → bl_op_erase_handler() → norflash.c
//   ├─ WRITE → bl_op_write_handler() → norflash.c  
//   ├─ VERIFY → bl_op_verify_handler()
//   └─ BOOT → bl_op_boot_handler() → boot_application()