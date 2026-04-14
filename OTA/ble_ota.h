/**
  * @file    ble_ota.h
  * @brief   通过蓝牙透传模块接收固件（自定义分包协议：包序号+CRC+数据）
  */

#ifndef __BLE_OTA_H
#define __BLE_OTA_H

#include "stm32f4xx.h"

#define BLE_PACKET_HEADER   4      /* 2字节包序号 + 2字节CRC16 */
#define BLE_MAX_DATA_LEN    508    /* 最大数据长度，保证总包大小不超过512 */

void BLE_OTA_Init(void);                   /* 初始化，擦除APP2区 */
void BLE_OTA_ReceivePacket(uint8_t *buf, uint32_t len); /* 接收到一包数据时调用（在串口中断中）*/
uint8_t BLE_OTA_IsComplete(void);          /* 返回是否完整接收并校验成功 */
uint32_t BLE_OTA_GetReceivedSize(void);    /* 获取已接收字节数 */

#endif

