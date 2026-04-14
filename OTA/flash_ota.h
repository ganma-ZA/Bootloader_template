/**
  * @file    flash_ota.h
  * @brief   Flash分区管理、参数存储、CRC校验
  * @author  OTA Template
  * @version 1.0
  */

#ifndef __FLASH_OTA_H
#define __FLASH_OTA_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <string.h>

/*-------------------------- 分区地址定义（STM32F407VG 512KB Flash）--------------------------*/
#define FLASH_BASE_ADDR         0x08000000      /* Flash基地址 */

/* 分区1：Bootloader区（32KB）*/
#define BOOTLOADER_ADDR         0x08000000
#define BOOTLOADER_SIZE         0x8000          /* 32KB */

/* 分区2：参数区（4KB，使用双备份存储）*/
#define PARAM_BLOCK0_ADDR       0x08008000      /* 参数块0起始地址 */
#define PARAM_BLOCK1_ADDR       0x08008400      /* 参数块1起始地址（备份）*/
#define PARAM_BLOCK_SIZE        0x400           /* 每个参数块1KB，足够存储结构体 */

/* 分区3：APP1区（运行区，238KB）*/
#define APP1_ADDR               0x08009000
#define APP1_MAX_SIZE           238 * 1024      /* 238KB */

/* 分区4：APP2区（下载暂存区，238KB）*/
#define APP2_ADDR               0x08049000
#define APP2_MAX_SIZE           238 * 1024      /* 238KB */

/*-------------------------- 参数区数据结构体（带CRC校验）----------------------------------*/
#pragma pack(1)   /* 紧凑排列，避免字节对齐干扰CRC计算 */
typedef struct {
    uint32_t magic;             /* 魔术字，固定为 0x5A5A5A5A，用于判断参数是否有效 */
    uint8_t  update_flag;       /* 升级标志：0=无升级任务，1=待升级，2=升级完成待重启校验 */
    uint8_t  channel;           /* 升级通道：1=串口(YModem)，2=4G(HTTP)，3=蓝牙 */
    uint16_t reserved;          /* 保留字节，对齐用 */
    
    uint8_t  current_ver[16];   /* 当前固件版本号，字符串格式如 "1.0.2" */
    uint8_t  new_ver[16];       /* 新固件版本号 */
    
    uint32_t firmware_size;     /* 新固件总字节数 */
    uint32_t firmware_crc32;    /* 新固件CRC32校验值 */
    
    char     firmware_url[128]; /* 4G升级时的下载URL */
    
    uint32_t param_crc32;       /* 整个结构体（不含本字段）的CRC32，用于校验参数完整性 */
} OTA_Param_t;
#pragma pack()

/*-------------------------- 外部接口函数 --------------------------------------------------*/
void     Flash_Init(void);                       /* 初始化Flash（解锁、清除标志）*/
void     Flash_DeInit(void);                     /* 关闭Flash（加锁）*/

uint8_t  Flash_EraseSector(uint32_t addr);       /* 擦除指定地址所在的扇区（自动计算扇区）*/
uint8_t  Flash_WriteWord(uint32_t addr, uint32_t data);   /* 写入一个字（32位）*/
uint8_t  Flash_WriteBuffer(uint32_t dest, uint8_t *src, uint32_t len); /* 写入任意长度数据 */

void     Flash_CopyPage(uint32_t src, uint32_t dst, uint32_t size); /* 将src区域数据拷贝到dst（页对齐）*/

/* 参数区读写（带双备份和CRC校验）*/
uint8_t  Param_Load(OTA_Param_t *param);         /* 从Flash加载有效参数（自动从两个块中选有效者）*/
uint8_t  Param_Save(OTA_Param_t *param);         /* 保存参数到Flash（更新双备份）*/
void     Param_ResetDefault(OTA_Param_t *param); /* 重置参数为默认值（magic=0x5A5A5A5A, update_flag=0等）*/

#endif /* __FLASH_OTA_H */

