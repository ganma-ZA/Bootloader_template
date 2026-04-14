#ifndef __PARAM_H
#define __PARAM_H

#pragma pack(1)
typedef struct {
    uint32_t magic;              // 魔术字，用于校验有效性（0x5A5A5A5A）
    uint8_t  update_flag;        // 升级标志：0-无升级 1-待升级
    uint8_t  reserved1;          
    uint16_t reserved2;
    uint8_t  version[8];         // 当前固件版本号
    uint8_t  new_version[8];     // 新固件版本号
    uint32_t firmware_size;      // 新固件大小
    uint32_t firmware_crc;       // 新固件CRC32校验值
    char     firmware_url[128];  // 4G升级用：固件下载URL
    uint32_t update_channel;     // 升级通道：1-串口 2-4G 3-蓝牙
    uint32_t param_crc;          // 整个结构体的CRC校验
} OTA_Param_t;
#pragma pack()
#define PARAM_ADDR  0x08008000    // 参数区起始地址
#define APP1_ADDR   0x08009000    // APP1区起始地址
#define APP2_ADDR   0x08049000    // APP2区起始地址

#endif 

