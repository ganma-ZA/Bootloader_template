/**
  * @file    ble_ota.c
  * @brief   蓝牙OTA协议接收
  */

#include "ble_ota.h"
#include "flash_ota.h"
#include "usart_driver.h"
#include <string.h>

/* 自定义协议包结构 */
typedef struct {
    uint16_t seq;       /* 包序号，从0开始 */
    uint16_t crc16;     /* 数据部分的CRC16 */
    uint8_t  data[BLE_MAX_DATA_LEN];
} BLE_Packet_t;

static uint16_t expect_seq = 0;
static uint32_t received_size = 0;
//static uint32_t total_size = 0;      /* 从第一个包或外部得知总大小 */
static uint8_t  upgrade_complete = 0;

/* 发送ACK（可通过蓝牙回复，这里假设直接发一个字节0x06）*/
static void BLE_SendAck(void) {
    /* 假设蓝牙串口为USART3 */
    USART_SendByte(USART3, 0x06);
}

static void BLE_SendNack(void) {
    USART_SendByte(USART3, 0x15);
}

void BLE_OTA_Init(void) {
    expect_seq = 0;
    received_size = 0;
    upgrade_complete = 0;
    Flash_Init();
    Flash_EraseSector(APP2_ADDR);
    Flash_DeInit();
}

/* 接收一个完整包（由外部串口中断组装好后调用）*/
void BLE_OTA_ReceivePacket(uint8_t *buf, uint32_t len) {
    if (len < BLE_PACKET_HEADER) {
        BLE_SendNack();
        return;
    }
    BLE_Packet_t *pkt = (BLE_Packet_t*)buf;
    uint16_t data_len = len - BLE_PACKET_HEADER;
    
    /* 校验包序号 */
    if (pkt->seq != expect_seq) {
        BLE_SendNack();
        return;
    }
    
    /* 计算数据CRC16（简单CCITT）*/
    uint16_t calc_crc = 0xFFFF;
    for (uint16_t i = 0; i < data_len; i++) {
        calc_crc ^= (uint16_t)pkt->data[i] << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
            else calc_crc <<= 1;
        }
    }
    if (calc_crc != pkt->crc16) {
        BLE_SendNack();
        return;
    }
    
    /* 写入Flash */
    Flash_Init();
    if (!Flash_WriteBuffer(APP2_ADDR + received_size, pkt->data, data_len)) {
        Flash_DeInit();
        BLE_SendNack();
        return;
    }
    Flash_DeInit();
    
    received_size += data_len;
    expect_seq++;
    BLE_SendAck();
    
    /* 如果外部已告知总大小，可判断完成 */
    /* 实际中可以通过第一个特殊包或外部参数传递总大小 */
}

uint8_t BLE_OTA_IsComplete(void) {
    return upgrade_complete;
}

uint32_t BLE_OTA_GetReceivedSize(void) {
    return received_size;
}

