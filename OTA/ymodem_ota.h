/**
  * @file    ymodem_ota.h
  * @brief   基于YModem协议的串口固件接收
  */

#ifndef __YMODEM_OTA_H
#define __YMODEM_OTA_H

#include "stm32f4xx.h"
#include <stdint.h>

void YModem_Init(void);                // 初始化YModem接收状态机
void YModem_ReceiveByte(uint8_t data); // 串口中断中喂入数据
uint8_t YModem_StartUpgrade(void);     // 启动YModem升级（阻塞等待完成）
uint8_t YModem_IsComplete(void);       // 查询是否接收完成
uint32_t YModem_GetFileSize(void);     // 获取接收到的固件总大小

#endif


