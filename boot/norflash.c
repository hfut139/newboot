#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx.h"
#include "norflash.h"

#define LOG_TAG "flash"
#define LOG_LVL LOG_LVL_INFO
#include "elog.h"

#define FLASH_ARG_ADDRESS 0x08000000

typedef struct
{
    uint32_t sector;
    uint32_t size;
}sector_desc_t;

//stm32f4 每个分区的大小描述
static const sector_desc_t sector_descs[]=
{
    {FLASH_Sector_0, 16*1024},
    {FLASH_Sector_1, 16*1024},
    {FLASH_Sector_2, 16*1024},
    {FLASH_Sector_3, 16*1024},
    {FLASH_Sector_4, 64*1024},
    {FLASH_Sector_5, 128*1024},
    {FLASH_Sector_6, 128*1024},
    {FLASH_Sector_7, 128*1024},
    {FLASH_Sector_8, 128*1024},
    {FLASH_Sector_9, 128*1024},
    {FLASH_Sector_10, 128*1024},
    {FLASH_Sector_11, 128*1024},
};

void bl_norflash_unclock(void)
{
    FLASH_Unlock();
}

void bl_norflash_clock(void)
{
    FLASH_Clock();
}

void bl_norflash_erase(uint32_t address,uint32_t size)
{
    uint32_t addr=FLASH_ARG_ADDRESS;
    for(int i=0;i<sizeof(sector_descs)/sizeof(sector_desc_t);i++)
    {
        if(addr>=address&&addr<(address+size))
        {
            log_i("erase sector %u,addr:0x%08x,size:%u",i,addr,sector_descs[i].size)

            if(FLASH_EraseSector(sector_descs[i].sector, VoltageRange_3) != FLASH_COMPLETE)
            {
                log_w("erase sector %u failed",i);  
            }
        }
        addr+=sector_descs[i].size;
    }
}

void bl_norflash_write(uint32_t address,uint8_t *data,uint32_t size)
{
    for(int i=0;i<size;i+=4)
    {
        if(FLASH_ProgramWord(address+i,*(uint32_t *)(data+i))!= FLASH_COMPLETE)
        {
            log_w("write data failed,addr:0x%08x",address+i); 
        }
    }
}