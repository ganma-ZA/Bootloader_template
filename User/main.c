/**
  * @file    bootloader_main.c
  * @brief   Bootloader入口，完成升级调度和跳转
  */

#include "stm32f4xx.h"
#include "OTA/jump_app.h"  // Removed due to missing header
#include "OTA/ymodem_ota.h"
#include "OTA/http_ota_4g.h"
#include "OTA/ble_ota.h"
#include "OTA/delay.h"
#include "OTA/crc32.h"
#include <string.h>


/* if the USART driver header is unavailable, declare the init function directly */
extern void USART1_Init(uint32_t baudrate);

/* Extern declaration for JumpToApp since jump_app.h is missing */
extern void JumpToApp(uint32_t addr);

/* LED指示函数 */
static void LED_ErrorBlink(void) {
    /* 简单闪烁表示错误 */
    while(1) {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        Delay_Ms(200);
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        Delay_Ms(200);
    }
}

/* 拷贝APP2固件到APP1区（在下载完成并校验通过后）*/
static uint8_t CopyFirmwareToApp1(uint32_t size) {
    Flash_Init();
    /* 擦除APP1区 */
    if (!Flash_EraseSector(APP1_ADDR)) return 0;
    /* 拷贝数据 */
    Flash_CopyPage(APP2_ADDR, APP1_ADDR, size);
    Flash_DeInit();
    return 1;
}

int main(void) {
    OTA_Param_t ota_param;
    
    /* 系统初始化 */
    SystemInit();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    Delay_Init();
    USART1_Init(115200);   /* 调试串口 */
    
    /* 加载参数 */
    if (!Param_Load(&ota_param)) {
        /* 参数无效，重置为默认值 */
        Param_ResetDefault(&ota_param);
        Param_Save(&ota_param);
    }
    
    /* 检查是否有待升级标志 */
    if (ota_param.update_flag == 1) {
        uint8_t upgrade_ok = 0;
        
        switch (ota_param.channel) {
            case 1:  /* 串口YModem升级 - disabled due to missing header */
                 USART1_Init(115200);
                 YModem_Init();
                 upgrade_ok = YModem_StartUpgrade();
                 if (upgrade_ok) {
                     /* 校验固件CRC */
                     uint32_t calc_crc = CRC32_Flash(APP2_ADDR, ota_param.firmware_size);
                     if (calc_crc == ota_param.firmware_crc32) {
                         upgrade_ok = CopyFirmwareToApp1(ota_param.firmware_size);
                     } else upgrade_ok = 0;
                 }
                break;
                
            case 2:  /* 4G HTTP升级 */
                if (EC200_Init()) {
                    if (EC200_DownloadFirmware(&ota_param)) {
                        upgrade_ok = CopyFirmwareToApp1(ota_param.firmware_size);
                    }
                }
                break;
                
            case 3:  /* 蓝牙升级 */
                BLE_OTA_Init();
                /* 等待蓝牙传输完成（实际可在中断中设置标志）*/
                while (!BLE_OTA_IsComplete()) {
                    Delay_Ms(10);
                }
                /* 校验CRC */
                uint32_t calc_crc = CRC32_Flash(APP2_ADDR, ota_param.firmware_size);
                if (calc_crc == ota_param.firmware_crc32) {
                    upgrade_ok = CopyFirmwareToApp1(ota_param.firmware_size);
                }
                break;
                
            default:
                break;
        }
        
        if (upgrade_ok) {
            /* 升级成功，清除标志，更新版本号 */
            ota_param.update_flag = 0;
            strcpy((char*)ota_param.current_ver, (char*)ota_param.new_ver);
            Param_Save(&ota_param);
        } else {
            /* 升级失败，不清除标志，可继续尝试或报错 */
            LED_ErrorBlink();
        }
    }
    
    /* 验证APP1区固件有效性，若有效则跳转 */
    uint32_t app1_stack = *(uint32_t*)APP1_ADDR;
    if (app1_stack >= 0x20000000 && app1_stack < 0x20020000) {
        JumpToApp(APP1_ADDR);
    }
    
    /* 若APP1无效，进入串口恢复模式 */
    while (1) {
        /* 可以通过YModem接收固件并直接写入APP1，然后跳转 - disabled due to missing header */
        // YModem_Init();
        // if (YModem_StartUpgrade()) {
        //     uint32_t calc_crc = CRC32_Flash(APP2_ADDR, ota_param.firmware_size);
        //     if (calc_crc == ota_param.firmware_crc32) {
        //         CopyFirmwareToApp1(ota_param.firmware_size);
        //         JumpToApp(APP1_ADDR);
        //     }
        // }
        Delay_Ms(1000);
    }
}


