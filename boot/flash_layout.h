#ifndef __FLASH_LAYOUT_H
#define __FLASH_LAYOUT_H

//1.���豸����ʱ���У���ʼ��Ӳ��
//2.�����ǽ���Ӧ�ó�����ִ����������(��̼�����)
#define FLASH_BOOT_ADDRESS 0x08000000 //�洢�����������ʼ��ַ
#define FLASH_BOOT_SIZE    48*1024 //�洢��������Ĵ�С


#define FLASH_ARG_ADDRESS 0x0800C000  //�洢У������
#define FLASH_ARG_SIZE    16*1024

#define FLASH_APP_ADDRESS 0x08010000 //�洢Ӧ�ó������ʼ��ַ
#define FLASH_APP_SIZE    256*1024 //�洢Ӧ�ó���Ĵ�С


#endif