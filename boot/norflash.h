#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include<stdint.h>


void bl_norflash_unclock(void);
void bl_norflash_clock(void);
void bl_norflash_erase(uint32_t address,uint32_t size);
void bl_norflash_write(uint32_t address,uint8_t *data,uint32_t size);


#endif /* __NORFLASH_H__ */