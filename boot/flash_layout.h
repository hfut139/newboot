#ifndef __FLASH_LAYOUT_H
#define __FLASH_LAYOUT_H

//1.在设备启动时运行，初始化硬件
//2.决定是进入应用程序还是执行其他任务(如固件升级)
#define FLASH_BOOT_ADDRESS 0x08000000 //存储引导程序的起始地址
#define FLASH_BOOT_SIZE    48*1024 //存储引导程序的大小


#define FLASH_ARG_ADDRESS 0x0800C000  //存储校验数据
#define FLASH_ARG_SIZE    16*1024

#define FLASH_APP_ADDRESS 0x08010000 //存储应用程序的起始地址
#define FLASH_APP_SIZE    256*1024 //存储应用程序的大小


#endif