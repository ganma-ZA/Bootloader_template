/**
  * @file    http_ota_4g.h
  * @brief   通过4G模块(EC200)进行HTTP下载固件
  * @note    需要根据实际模块AT指令集调整
  */

#ifndef __HTTP_OTA_4G_H
#define __HTTP_OTA_4G_H

#include "stm32f4xx.h"
#include "flash_ota.h"

/* 模块初始化 */
uint8_t EC200_Init(void);                     /* 初始化4G模块，拨号上线 */
uint8_t EC200_CheckNewVersion(OTA_Param_t *param); /* 从服务器获取最新版本信息，如有更新则填充param并置update_flag */
uint8_t EC200_DownloadFirmware(OTA_Param_t *param); /* 根据param中的URL下载固件到APP2区，并校验CRC */

#endif

