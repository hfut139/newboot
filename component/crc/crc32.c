#include "crc32.h"


#define CRC32_TABLE_SIZE    0x100  //256
//CRC32表大小（256项）
//CRC32表的大小是256，因为CRC-32算法处理的是8位字节数据，每个字节对应一个CRC值。
//CRC32表的大小决定了查找表的大小，256项可以覆盖所有可能的8位字节值（0x00到0xFF）。


static uint8_t inited = 0;  //初始化标志变量
static uint32_t crc32_table[CRC32_TABLE_SIZE];


// 逐字节计算CRC32表
//CRC 的本质：将数据视为二进制多项式，与固定生成多项式（如 0x04C11DB7）进行模 2 除法，余数即为校验值。
//反转多项式：0xedb88320 是标准多项式 0x04C11DB7 的位反转（Bit-reversed）形式（方便从 LSB 开始处理）。
static uint32_t crc32_for_byte(uint32_t r)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        r = (r & 1 ? 0 : (uint32_t)0xedb88320) ^ r >> 1;
    }

    return r ^ (uint32_t)0xff000000;
}
/*
检查最低位（LSB）：

如果 LSB 为 1：

当前数据多项式“可被”生成多项式整除（模 2 减法）。

异或 0 相当于直接右移（跳过减法）。

如果 LSB 为 0：

需要“借位”异或生成多项式（模 2 减法）。

异或 0xedb88320 实现减法。

右移 r >> 1：

相当于将数据多项式除以 x（即下一次处理更高位）。

示例说明
假设当前 r = 0x03（00000011）：

第一次循环（LSB=1）：

直接右移：r = 0x03 >> 1 → 0x01（相当于跳过减法）。

第二次循环（LSB=1）：

直接右移：r = 0x01 >> 1 → 0x00。

第三次循环（LSB=0）：

异或多项式并右移：r = 0xedb88320 ^ (0x00 >> 1) → 0xedb88320


为什么最后要异或 0xff000000？
这是 CRC-32 算法的输出补码（Final XOR），目的是：

反转输出位序：

某些 CRC 标准（如 CRC-32/IEEE 802.3）要求最终结果进行位反转。

0xff000000 的高 8 位是 1，异或后会反转结果的最高有效位（MSB）。

避免全零冲突：

如果不做最终异或，输入全零数据的 CRC 结果也是全零，可能掩盖错误。

异或 0xff000000 确保全零输入产生非零输出。

标准化输出：

符合常见 CRC-32 实现（如 ZIP、PNG 文件使用的 CRC）。
*/

// 生成CRC32表，内存和时间换取空间
void crc32_init(void)
{
    for (uint32_t i = 0; i < CRC32_TABLE_SIZE; ++i)
    {
        crc32_table[i] = crc32_for_byte(i);
    }

    inited = 1;
}

uint32_t crc32_update(uint32_t crc, uint8_t *data, uint32_t len)
{
    if (!inited)
    {
        crc32_init();
    }

    for (uint32_t i = 0; i < len; i++)
    {
        crc = crc32_table[(uint8_t)crc ^ ((uint8_t *)data)[i]] ^ crc >> 8;
    }

    return crc;
}
/*
示例数据
假设我们要校验 2 字节的数据：0x01 0x02（二进制 00000001 00000010）。

1. 预计算 CRC 表（Lookup Table）
首先用 crc32_for_byte 函数生成 256 项的 CRC 表（每个字节对应一个 CRC 值）。以 0x01 为例（前文已计算）：

输入字节	CRC 值（crc32_for_byte 输出）
0x01	0xa5041ddb
0x02	0x4d07f6b6（计算过程类似）
2. CRC 校验计算流程
CRC-32 校验的核心是 逐字节查表并更新 CRC 值。步骤如下：

初始化 CRC 值
c
uint32_t crc = 0xFFFFFFFF;  // 初始值（通常为全 1）
处理第一个字节 0x01
查表：取 0x01 对应的 CRC 值 0xa5041ddb。

更新 CRC：

异或操作：crc ^= 0x01 → 0xFFFFFFFF ^ 0x00000001 = 0xFFFFFFFE。

查表混合：

取 crc 的低 8 位：0xFE。

查表得 crc_table[0xFE]（假设为 0x8a62a0a0）。

移位和混合：

c
crc = (crc >> 8) ^ crc_table[0xFE];  // 右移 8 位后异或查表值
计算：

crc >> 8 → 0xFFFFFFFE >> 8 → 0x00FFFFFF。

0x00FFFFFF ^ 0x8a62a0a0 → 0x8a9d5f5f（新 crc）。

处理第二个字节 0x02
查表：取 0x02 对应的 CRC 值 0x4d07f6b6。

更新 CRC：

异或操作：crc ^= 0x02 → 0x8a9d5f5f ^ 0x00000002 = 0x8a9d5f5d。

查表混合：

取 crc 的低 8 位：0x5D。

查表得 crc_table[0x5D]（假设为 0x3b6e20c8）。

移位和混合：

c
crc = (crc >> 8) ^ crc_table[0x5D];
计算：

crc >> 8 → 0x008a9d5f。

0x008a9d5f ^ 0x3b6e20c8 → 0x33e4bd97（新 crc）。

最终 CRC 值
c
crc ^= 0xFFFFFFFF;  // 取反（某些标准要求）
计算：0x33e4bd97 ^ 0xFFFFFFFF → 0xcc1b4268。

3. 校验过程总结
步骤	操作	中间 CRC 值
初始化	crc = 0xFFFFFFFF	0xFFFFFFFF
处理 0x01	查表、异或、混合	0x8a9d5f5f
处理 0x02	查表、异或、混合	0x33e4bd97
最终取反	crc ^ 0xFFFFFFFF	0xcc1b4268


1. CRC 校验值的本质
CRC（Cyclic Redundancy Check）是一种 错误检测码，用于验证数据在传输或存储过程中是否被意外修改（如位翻转、丢失、插入等）。

0xcc1b4268 是数据 0x01 0x02 的唯一“指纹”：任何微小的数据变动（如将 0x01 改为 0x02）都会导致 CRC 值完全不同。

数学原理：本质是数据多项式除以生成多项式（如 0x04C11DB7）的余数。

2. 为什么是这个值？
算法决定：
CRC-32 的结果取决于：

生成多项式（如 0x04C11DB7 或其反转形式 0xEDB88320）。

初始值（如 0xFFFFFFFF）。

最终异或值（如 0xFFFFFFFF）。

输入数据的位序（LSB 或 MSB 优先）。

你的例子：
使用反转多项式（0xEDB88320）和初始值 0xFFFFFFFF，最终异或 0xFFFFFFFF，因此得到 0xcc1b4268。

3. 实际应用场景
场景 1：数据传输校验
发送方：计算数据的 CRC 值（如 0xcc1b4268），附加到数据末尾发送。

接收方：重新计算 CRC，若与接收到的 CRC 不匹配，说明数据出错。

场景 2：文件完整性验证
ZIP/PNG 文件：文件尾部存储 CRC-32 值，解压时校验文件是否损坏。

例如：若文件内容被篡改，CRC 值会变化，解压软件报错。

场景 3：硬件错误检测
网络协议（如 Ethernet）：数据帧的 CRC 校验字段用于检测传输错误。

4. 为什么不用其他值（如 MD5/SHA）？
CRC 的特点：

速度快：适合实时性高的场景（如网络传输）。

轻量级：计算资源占用低（硬件易实现）。

检测随机错误：对突发位错误敏感（但不如哈希算法抗故意篡改）。

对比：

MD5/SHA：用于加密哈希，计算复杂，但能防恶意篡改。

CRC：仅防意外错误，不防攻击。

*/
