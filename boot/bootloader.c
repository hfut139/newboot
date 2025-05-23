#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "main.h"
#include "crc32.h"
#include "ringbuffer8.h"
#include "flash_layout.h"
#include "norflash.h"
#include "arginfo.h"
#include "uart.h"
#include "stm32f4xx.h"
#include "led.h"
#include "button.h"


#define LOG_LVL LOG_LVL_INFO
#define LOG_TAG "boot"
#include "elog.h"


//bootloader�汾��
#define BOOTLOADER_VERSION_MAJOR 1
#define BOOTLOADER_VERSION_MINOR 0

//���ݰ����ճ�ʱʱ��(ms)
#define BL_TIMEOUT_MS           500ul
//UART���ջ�������С
#define BL_UART_BUFFER_SIZE     512ul
//���ݰ�ͷ����С
#define BL_PACKET_HEAD_SIZE     8ul
//���ݰ�payload��С
#define BL_PACKET_PAYLOAD_SIZE  4096ul
//���ݰ���󳤶�
#define BL_PACKET_PARAM_SIZE  (BL_PACKET_HEAD_SIZE + BL_PACKET_PAYLOAD_SIZE)

/* format
 *
 * | start | opcode | length | payload | crc32 |
 * | u8    | u8     | u16    | u8 * n  | u32   |
 *
 * start: 0xAA
 */

 //���ݰ�����״̬��
 typedef enum
 {
    BL_SM_IDLE,
    BL_SM_START,
    BL_SM_OPCODE,
    BL_SM_LENGTH,
    BL_SM_PARAM,
    BL_SM_CRC,
 }bl_state_machine_t;

 //���ݰ�������
 typedef enum
 {
    BL_OP_NONE=0x00,
    BL_OP_INQUIRY=0x10,
    BL_OP_BOOT=0x11,
    BL_OP_RESET=0x1F,
    BL_OP_ERASE=0x20,
    BL_OP_READ, //����δʵ��
    BL_OP_WRITE,
    BL_OP_VERIFY,
    BL_OP_END,
 }bl_op_t;

 typedef enum
 {
    BL_INQUIRY_VERSION,
    BL_INQUIRY_MTU_SIZE,
 }bl_inquiry_t;

 //������
 typedef enum
 {
    BL_ERR_OK,
    BL_ERR_OPCODE,
    BL_ERR_OVERFLOW,
    BL_ERR_TIMEOUT,
    BL_ERR_FORMAT,
    BL_ERR_VERIFY,
    BL_ERR_PARAM,
    BL_ERR_UNKNOWN=0xff,
 }bl_err_t;

 //���ݰ�����
 typedef struct 
 {
    bl_op_t opcode;
    uint16_t length;
    uint32_t crc;

    uint8_t param[BL_PACKET_PAYLOAD_SIZE];
    uint16_t index;
 }bl_pkt_t;

 //���ݰ����ջ�����
 typedef struct
 {
   uint8_t data[16];
   uint16_t index; //��ǰ���ݳ���
 }bl_rx_t;

 //bootloader���ݿ�����
 typedef struct
 {
    bl_pkt_t pkt;
    bl_rx_t rx;
    bl_state_machine_t sm;
 }bl_ctrl_t;
 
//inquery opcode ��param���ݽṹ
typedef struct
{
    uint8_t subcode;
}bl_inquiry_param_t;

//erase opcode ��param���ݽṹ
typedef struct
{
    uint32_t address;
    uint32_t size;
}bl_erase_param_t;
