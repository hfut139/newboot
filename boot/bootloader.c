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


//bootloader版本号
#define BOOTLOADER_VERSION_MAJOR 1
#define BOOTLOADER_VERSION_MINOR 0

//数据包接收超时时间(ms)
#define BL_TIMEOUT_MS           500ul
//UART接收缓冲区大小
#define BL_UART_BUFFER_SIZE     512ul
//数据包头部大小
#define BL_PACKET_HEAD_SIZE     8ul
//数据包payload大小
#define BL_PACKET_PAYLOAD_SIZE  4096ul
//数据包最大长度
#define BL_PACKET_PARAM_SIZE  (BL_PACKET_HEAD_SIZE + BL_PACKET_PAYLOAD_SIZE)

/* format
 *
 * | start | opcode | length | payload | crc32 |
 * | u8    | u8     | u16    | u8 * n  | u32   |
 *
 * start: 0xAA
 */

 //数据包接收状态机
 typedef enum
 {
    BL_SM_IDLE,
    BL_SM_START,
    BL_SM_OPCODE,
    BL_SM_LENGTH,
    BL_SM_PARAM,
    BL_SM_CRC,
 }bl_state_machine_t;

 //数据包操作码
 typedef enum
 {
    BL_OP_NONE=0x00,
    BL_OP_INQUIRY=0x10,
    BL_OP_BOOT=0x11,
    BL_OP_RESET=0x1F,
    BL_OP_ERASE=0x20,
    BL_OP_READ, //功能无需实现
    BL_OP_WRITE,
    BL_OP_VERIFY,
    BL_OP_END,
 }bl_op_t;

 typedef enum
 {
    BL_INQUIRY_VERSION,
    BL_INQUIRY_MTU_SIZE,
 }bl_inquiry_t;

 //错误码
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

 //数据包描述
 typedef struct 
 {
    bl_op_t opcode;
    uint16_t length;
    uint32_t crc;

    uint8_t param[BL_PACKET_PAYLOAD_SIZE];
    uint16_t index;
 }bl_pkt_t;

 //数据包接收缓冲器
 typedef struct
 {
   uint8_t data[16];//这里不建议写成uint16_t,因为uint8_t是单字节对齐，uint16_t是2字节对齐
   uint16_t index; //当前数据长度
 }bl_rx_t;

 //bootloader数据控制器
 typedef struct
 {
    bl_pkt_t pkt;
    bl_rx_t rx;
    bl_state_machine_t sm;
 }bl_ctrl_t;
 
//inquery opcode 的param数据结构
typedef struct
{
    uint8_t subcode;
}bl_inquiry_param_t;

//erase opcode 的param数据结构
typedef struct
{
    uint32_t address;
    uint32_t size;
}bl_erase_param_t;

//read opcode 的param数据结构
typedef struct
{
    uint32_t address;
    uint32_t size;
}bl_read_param_t;

//write opcode 的param数据结构
typedef struct
{
    uint32_t address;
    uint32_t size;
    uint8_t data[];
}bl_write_param_t;

//verify opcode 的param数据结构
typedef struct
{
    uint32_t address;
    uint32_t size;
    uint32_t crc;
}bl_verify_param_t;

//串口接收ringbuffer
static ringbuffer8_t serial_rb;
static uint8_t serial_rb_buffer[BL_UART_BUFFER_SIZE];
static bl_ctrl_t bl_ctrl;
//上一次接收到数据包的时间，用于数据包接收超时检查
static uint32_t last_pak_time;



static void serial_recv_callback(uint8_t *data,uint32_t len)
{
    rb8_puts(serial_rb,data,len);
}

static void bl_reset(bl_ctrl_t *ctrl)
{
    ctrl->sm= BL_SM_IDLE;
    ctrl->rx.index=0;
    ctrl->pkt.index=0;
}

static void bl_response(bl_op_t op,uint8_t *data,uint16_t length)
{
    const uint8_t head=0xAA;

    uint32_t crc=0;
    crc=crc32_update(crc,(uint8_t *)&head,1);
    crc=crc32_update(crc,(uint8_t)&op,1);
    crc=crc32_update(crc,(uint8_t *)&length,2);
    crc=crc32_update(crc,data,length);

    bl_uart_write((uint8_t *)&head,1);
    bl_uart_write((uint8_t *)&op,1);
    bl_uart_write((uint8_t *)&length,2);
    bl_uart_write(data,length);
    bl_uart_write((uint8_t *)&crc,4);
}

static void bl_response_ack(bl_op_t op,bl_err_t err)
{
    bl_response(op,(uint8_t *)&err,1);
}

static void bl_op_inquiry_handler(uint8_t *data,uint16_t length)
{
    log_i("inquery");

    //将param数据强制转换为bl_inquiry_param_t结构体
    //因为param内的数据就是bl_inquiry_param_t类型
    bl_inquiry_param_t *inquery=(void *)data;

    //如果数据长度不等于bl_inquiry_param_t的长度，说明数据包格式错误
    if(length!=sizeof(bl_inquiry_param_t))
    {
        log_w("length mismatch %d!=%d",length,sizeof(bl_inquiry_param_t));
        bl_response_ack(BL_OP_INQUIRY,BL_ERR_PARAM);
        return;
    }

    log_i("subcode:0x%02x",inquery->subcode);
    //根据subcode调用不同的处理函数
    switch(inquery->subcode)
    {
        //查询Bootloader版本号
        case BL_INQUIRY_VERSION:
        {
            uint8_t version[]={BOOTLOADER_VERSION_MAJOR,BOOTLOADER_VERSION_MINOR};
            bl_response(BL_OP_INQUIRY,version,sizeof(version));
            break;
        }
        //查询MTU大小
        case BL_INQUIRY_MTU_SIZE:
        {
            uint16_t size=BL_PACKET_PAYLOAD_SIZE;
            bl_response(BL_OP_INQUIRY,(uint8_t *)&size,sizeof(size));
            break;
        }
        //其它subcode,返回错误
        default:
        {
            bl_response_ack(BL_OP_INQUIRY,BL_ERR_PARAM);
            break;
        }
    }
}

//直接去引导主程序
static void bl_op_boot_handler(uint8_t *data,uint16_t length)
{
    log_i("boot");

    bl_response_ack(BL_OP_BOOT,BL_ERR_OK);

    //boot_application();
}

//重启系统
static void bl_op_reset_handler(uint8_t *data,uint16_t length)
{
    log_i("reset");

    bl_response_ack(BL_OP_RESET,BL_ERR_OK);

    NVIC_SystemReset(); //重启系统
}

static void bl_op_erase_handler(uint8_t *data,uint16_t length)
{
    log_i("erase");

    bl_erase_param_t *erase=(void *)data;

    if(length!=sizeof(bl_erase_param_t))
    {
        log_w("length mismatch %d!=%d",length,sizeof(bl_erase_param_t));
        bl_response_ack(BL_OP_ERASE,BL_ERR_PARAM);
        return;
    }

    //防止擦除bootloader区域
    if(erase->address>=FLASH_BOOT_ADDRESS&&erase->address<(FLASH_BOOT_ADDRESS+FLASH_BOOT_SIZE))
    {
        log_w("address 0x%08x is protected",erase->address);
        bl_response_ack(BL_OP_ERASE,BL_ERR_PARAM);
        return;
    }

    log_i("erase address:0x%08x,size:%d",erase->address,erase->size);

    //操作flash前需解锁
    bl_norflash_unclock();
    bl_norflash_erase(erase->address, erase->size);
    bl_norflash_clock();

    bl_response_ack(BL_OP_ERASE,BL_ERR_OK);
}

//bootloader无需实现read功能
static void bl_op_read_handler(uint8_t *data,uint16_t length)
{
    log_i("read");

    // bl_read_param_t *read=(void *)data;

    // if(length!=sizeof(bl_read_param_t))
    // {
    //     log_w("length mismatch %d!=%d",length,sizeof(bl_read_param_t));
    //     bl_response_ack(BL_OP_READ,BL_ERR_PARAM);
    //     return;
    // }

    // log_i("read address:0x%08x,size:%d",read->address,read->size);
}

//写入数据到flash
static void bl_op_write_handler(uint8_t *data,uint16_t length)
{
    log_i("write");

    bl_write_param_t *write=(void *)data;

    if(length!=sizeof(bl_write_param_t))
    {
        log_w("length mismatch %d!=%d",length,sizeof(bl_write_param_t));
        bl_response_ack(BL_OP_WRITE,BL_ERR_PARAM);
        return;
    }

    if(write->address>=FLASH_BOOT_ADDRESS && write->address<(FLASH_BOOT_ADDRESS+FLASH_BOOT_SIZE))
    {
        log_w("address 0x%08x is protected",write->address);
        bl_response_ack(BL_OP_ERASE,BL_ERR_UNKNOWN);
        return;
    }

    log_i("write:0x%08x,size:%d",write->address,write->size);
    bl_norflash_unclock();
    bl_norflash_write(write->address, write->data, write->size);
    bl_norflash_clock();
    bl_response_ack(BL_OP_WRITE,BL_ERR_OK);
}

static void bl_op_verify_handler(uint8_t *data,uint16_t length)
{
    log_i("verity");
    bl_verify_param_t *verity=(void *)data;
    if(length!=sizeof(bl_verify_param_t))
    {
        log_w("length mismatch %d!=%d",length,sizeof(bl_verify_param_t));
        bl_response_ack(BL_OP_VERIFY,BL_ERR_PARAM);
        return;
    }

    log_i("verity:0x%08x,size:%d",verity->address,verity->size);
    uint32_t crc=crc32_update(0,verity->address,verity->size);

    log_i("crc:%08x,verity:%08x",crc,verity->crc);
    if(crc==verity->crc)
    {
        bl_response_ack(BL_OP_VERIFY,BL_ERR_OK);
    }
    else
    {
        bl_response_ack(BL_OP_VERIFY,BL_ERR_VERIFY);
    }
}

static void bl_pkt_handler(bl_pkt_t *pkt)
{
    log_i("opcode:0x%02x,length:%d",pkt->opcode,pkt->length);

    //根据opcode调用不同的处理函数
    switch (pkt->opcode)
    {
        case BL_OP_INQUIRY:
            bl_op_inquiry_handler(pkt->param,pkt->length);
            break;
        case BL_OP_BOOT:
            bl_op_boot_handler(pkt->param,pkt->length);
            break;
        case BL_OP_RESET:
            bl_op_reset_handler(pkt->param,pkt->length);
            break;
        case BL_OP_ERASE:
            bl_op_erase_handler(pkt->param,pkt->length);
            break;
        case BL_OP_READ:
            bl_op_read_handler(pkt->param,pkt->length);
            break;
        case BL_OP_WRITE:
            bl_op_write_handler(pkt->param,pkt->length);
            break;
        case BL_OP_VERIFY:
            bl_op_verify_handler(pkt->param,pkt->length);
            break;
        default:
            break;
    }
}

static bool bl_pkt_verity(bl_pkt_t *pkt)
{
    const uint8_t head=0xAA;

    uint32_t crc=0;
    crc=crc32_update(crc,(uint8_t *)&head,1);
    crc=crc32_update(crc,(uint8_t *)&pkt->opcode,1);
    crc=crc32_update(crc,(uint8_t *)&pkt->length,2);
    crc=crc32_update(crc,pkt->param,pkt->length);

    return crc==pkt->crc;
}

static bool bl_recv_handler(bl_ctrl_t *ctrl,uint8_t data)
{
    bool fullpkt=false;

    bl_rx_t *rx=&ctrl->rx;
    bl_pkt_t *pkt=&ctrl->pkt;

    //把接收到的数据放到缓存里面
    rx->data[rx->index++]=data;

    //根据状态机处理帧内接收阶段
    switch(ctrl->sm)
    {
        //当前处于IDLE模式，等待接收0xAA
        case BL_SM_IDLE:
        {
            log_d("sm idle");

            //不论是否收到0xAA，都将index置为0，以便在状态机切换时，数据缓存器清空
            rx->index=0;
            //如果接收到0xAA，则切换到BL_SM_START状态
            if(rx->data[0]==0xAA)
            {
                ctrl->sm=BL_SM_START;
            }
            break;
        }
        //当前处于start模式，收到opcode
        case BL_SM_START:
        {
            log_d("sm start");

            rx->index=0; //清空数据缓存器
            pkt->opcode=(bl_op_t)rx->data[0];
            ctrl->sm=BL_SM_OPCODE;
            break;
        }
        //当前处于opcode模式，收到length
        case BL_SM_OPCODE:
        {
            log_d("sm opcode");

            //因为length是2字节，所以index等于2时，表示length已经接收完毕
            if(rx->index==2)
            {
                rx->index=0;
                uint16_t length=*(uint16_t *)(rx->data);
            }
        }

    }
}