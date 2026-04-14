/**
  * @file    flash_ota.c
  * @brief   Flash擦写、参数区双备份读写、CRC32计算
  */

#include "flash_ota.h"
#include "crc32.h"   /* 包含CRC32表 */

/* 静态函数：计算参数结构体的CRC（不包含param_crc32字段）*/
static uint32_t Param_CalcCRC(const OTA_Param_t *param) {
    /* 计算时排除最后4字节的param_crc32 */
    return CRC32_Calculate((const uint8_t*)param, sizeof(OTA_Param_t) - 4);
}

/* 检查参数块是否有效（magic正确且CRC校验通过）*/
static uint8_t Param_IsBlockValid(uint32_t block_addr) {
    OTA_Param_t param;
    uint32_t *src = (uint32_t*)block_addr;
    uint32_t *dst = (uint32_t*)&param;
    for (uint32_t i = 0; i < sizeof(OTA_Param_t)/4; i++) {
        dst[i] = src[i];
    }
    if (param.magic != 0x5A5A5A5A) return 0;
    uint32_t calc_crc = Param_CalcCRC(&param);
    return (calc_crc == param.param_crc32);
}

/* 擦除一个扇区（STM32F4每个扇区大小不等，自动判断）*/
uint8_t Flash_EraseSector(uint32_t addr) {
    uint32_t sector_num;
    uint32_t offset = addr - FLASH_BASE_ADDR;
    
    /* STM32F40x扇区映射：0-16KB,1-16KB,2-16KB,3-16KB,4-64KB,5-128KB,6-128KB,7-128KB,... */
    if (offset < 16*1024) sector_num = FLASH_Sector_0;
    else if (offset < 32*1024) sector_num = FLASH_Sector_1;
    else if (offset < 48*1024) sector_num = FLASH_Sector_2;
    else if (offset < 64*1024) sector_num = FLASH_Sector_3;
    else if (offset < 128*1024) sector_num = FLASH_Sector_4;
    else if (offset < 256*1024) sector_num = FLASH_Sector_5;
    else if (offset < 384*1024) sector_num = FLASH_Sector_6;
    else sector_num = FLASH_Sector_7;
    
    FLASH_Status status = FLASH_EraseSector(sector_num, VoltageRange_3);
    return (status == FLASH_COMPLETE);
}

/* 写入一个字（必须处于解锁状态，地址必须字对齐）*/
uint8_t Flash_WriteWord(uint32_t addr, uint32_t data) {
    FLASH_Status status = FLASH_ProgramWord(addr, data);
    return (status == FLASH_COMPLETE);
}

/* 写入任意长度数据（自动按字拆分）*/
uint8_t Flash_WriteBuffer(uint32_t dest, uint8_t *src, uint32_t len) {
    uint32_t words = (len + 3) / 4;
    uint32_t *p_src = (uint32_t*)src;
    for (uint32_t i = 0; i < words; i++) {
        if (!Flash_WriteWord(dest + i*4, p_src[i])) {
            return 0;
        }
    }
    return 1;
}

/* 拷贝Flash区域（注意：目的区域必须先擦除）*/
void Flash_CopyPage(uint32_t src, uint32_t dst, uint32_t size) {
    uint32_t words = (size + 3) / 4;
    for (uint32_t i = 0; i < words; i++) {
        uint32_t data = *(uint32_t*)(src + i*4);
        Flash_WriteWord(dst + i*4, data);
    }
}

/* 初始化Flash外设（解锁、清除错误标志）*/
void Flash_Init(void) {
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                    FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
}

/* 关闭Flash（加锁）*/
void Flash_DeInit(void) {
    FLASH_Lock();
}

/* 加载参数：优先使用块0，如果块0无效则尝试块1，都无效则返回0 */
uint8_t Param_Load(OTA_Param_t *param) {
    if (Param_IsBlockValid(PARAM_BLOCK0_ADDR)) {
        uint32_t *src = (uint32_t*)PARAM_BLOCK0_ADDR;
        uint32_t *dst = (uint32_t*)param;
        for (uint32_t i = 0; i < sizeof(OTA_Param_t)/4; i++) {
            dst[i] = src[i];
        }
        return 1;
    }
    else if (Param_IsBlockValid(PARAM_BLOCK1_ADDR)) {
        uint32_t *src = (uint32_t*)PARAM_BLOCK1_ADDR;
        uint32_t *dst = (uint32_t*)param;
        for (uint32_t i = 0; i < sizeof(OTA_Param_t)/4; i++) {
            dst[i] = src[i];
        }
        return 1;
    }
    return 0;
}

/* 保存参数：先擦除两个块，再写入新数据（双备份保证可靠性）*/
uint8_t Param_Save(OTA_Param_t *param) {
    /* 先计算当前参数的CRC（排除param_crc32字段）*/
    param->param_crc32 = Param_CalcCRC(param);
    
    Flash_Init();
    /* 擦除块0和块1 */
    if (!Flash_EraseSector(PARAM_BLOCK0_ADDR)) return 0;
    if (!Flash_EraseSector(PARAM_BLOCK1_ADDR)) return 0;
    
    /* 写入块0 */
    uint8_t *p = (uint8_t*)param;
    if (!Flash_WriteBuffer(PARAM_BLOCK0_ADDR, p, sizeof(OTA_Param_t))) return 0;
    /* 写入块1备份 */
    if (!Flash_WriteBuffer(PARAM_BLOCK1_ADDR, p, sizeof(OTA_Param_t))) return 0;
    
    Flash_DeInit();
    return 1;
}

/* 重置参数为默认值（用于初次烧录或参数损坏时）*/
void Param_ResetDefault(OTA_Param_t *param) {
    memset(param, 0, sizeof(OTA_Param_t));
    param->magic = 0x5A5A5A5A;
    param->update_flag = 0;
    param->channel = 0;
    strcpy((char*)param->current_ver, "1.0.0");
    strcpy((char*)param->new_ver, "");
    param->firmware_size = 0;
    param->firmware_crc32 = 0;
    param->firmware_url[0] = '\0';
    param->param_crc32 = Param_CalcCRC(param);
}


