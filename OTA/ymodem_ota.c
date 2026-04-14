/**
  * @file    ymodem_ota.c
  * @brief   YModem协议接收机实现（支持128字节和1024字节包）
  */

#include "ymodem_ota.h"
#include "flash_ota.h"
#include "usart_driver.h"
#include "delay.h"
#include <string.h>

/* YModem协议控制字符 */
#define SOH     0x01
#define STX     0x02
#define EOT     0x04
#define ACK     0x06
#define NAK     0x15
#define CAN     0x18
#define C       0x43

/* 状态机状态 */
typedef enum {
    YM_STATE_WAIT_START,
    YM_STATE_RECV_DATA,
    YM_STATE_COMPLETE,
    YM_STATE_CANCEL
} YM_State_t;

/* 全局变量 */
static YM_State_t ym_state = YM_STATE_WAIT_START;
static uint32_t ym_file_size = 0;
static uint32_t ym_received = 0;
static uint32_t ym_write_addr = APP2_ADDR;
static uint8_t  ym_packet_buf[1024+6];
static uint8_t  ym_packet_len = 0;
static uint8_t  ym_expect_seq = 0;
static uint8_t  ym_first_packet = 1;

/* 静态函数：发送一个字节 */
static void YM_SendByte(uint8_t ch) {
    USART_SendByte(USART1, ch);
}

static void YM_SendAck(void) {
    YM_SendByte(ACK);
}

static void YM_SendNak(void) {
    YM_SendByte(NAK);
}

static void YM_SendC(void) {
    YM_SendByte(C);
}

static void YM_SendCancel(void) {
    YM_SendByte(CAN);
    YM_SendByte(CAN);
}

/* 解析文件名包，获取文件大小 */
static uint8_t YM_ParseFileNamePacket(uint8_t *buf, uint32_t len, uint32_t *file_size) {
    uint8_t *p = buf + 3;  // 跳过包头和两个序号字节
    while (*p != 0x00 && (p - buf) < (int)len - 3) p++;
    if (*p != 0x00) return 0;
    p++;
    if (*p == 0x00) {
        *file_size = 0;
        return 1;
    }
    *file_size = 0;
    while (*p >= '0' && *p <= '9') {
        *file_size = (*file_size) * 10 + (*p - '0');
        p++;
    }
    return 1;
}

/* 写入数据包到Flash */
static uint8_t YM_WriteDataPacket(uint8_t *buf, uint32_t data_len) {
    uint8_t *data = buf + 3;
    if (!Flash_WriteBuffer(ym_write_addr + ym_received, data, data_len)) {
        return 0;
    }
    ym_received += data_len;
    return 1;
}

/* 初始化YModem接收 */
void YModem_Init(void) {
    ym_state = YM_STATE_WAIT_START;
    ym_file_size = 0;
    ym_received = 0;
    ym_packet_len = 0;
    ym_expect_seq = 0;
    ym_first_packet = 1;
}

/* 串口中断中喂入数据 */
void YModem_ReceiveByte(uint8_t data) {
    static uint8_t packet_number = 0;
    static uint8_t packet_crc1, packet_crc2;
    static uint16_t calc_crc;

    switch (ym_state) {
        case YM_STATE_WAIT_START:
            YM_SendC();
            ym_state = YM_STATE_RECV_DATA;
            ym_packet_len = 0;
            ym_expect_seq = 0;
            ym_first_packet = 1;
            break;

        case YM_STATE_RECV_DATA:
            ym_packet_buf[ym_packet_len++] = data;
            if (ym_packet_len >= 3) {
                uint8_t pkt_type = ym_packet_buf[0];
                uint8_t seq = ym_packet_buf[1];
                uint8_t seq_compl = ym_packet_buf[2];
                uint32_t data_len = 0;

                if (pkt_type == SOH) data_len = 128;
                else if (pkt_type == STX) data_len = 1024;
                else if (pkt_type == EOT) {
                    YM_SendAck();
                    ym_state = YM_STATE_COMPLETE;
                    return;
                }
                else if (pkt_type == CAN) {
                    static uint8_t can_count = 0;
                    if (++can_count >= 2) {
                        ym_state = YM_STATE_CANCEL;
                    }
                    return;
                }
                else {
                    YM_SendNak();
                    ym_packet_len = 0;
                    return;
                }

                if (seq != ym_expect_seq || (seq_compl != (0xFF - seq))) {
                    YM_SendNak();
                    ym_packet_len = 0;
                    return;
                }

                uint32_t total_len = 3 + data_len + 2;
                if (ym_packet_len >= total_len) {
                    /* 计算CRC16 */
                    calc_crc = 0xFFFF;
                    for (uint32_t i = 0; i < 3 + data_len; i++) {
                        calc_crc ^= (uint16_t)ym_packet_buf[i] << 8;
                        for (uint8_t bit = 0; bit < 8; bit++) {
                            if (calc_crc & 0x8000) calc_crc = (calc_crc << 1) ^ 0x1021;
                            else calc_crc <<= 1;
                        }
                    }
                    packet_crc1 = ym_packet_buf[3+data_len];
                    packet_crc2 = ym_packet_buf[3+data_len+1];
                    if ((calc_crc & 0xFF) != packet_crc1 || ((calc_crc >> 8) & 0xFF) != packet_crc2) {
                        YM_SendNak();
                        ym_packet_len = 0;
                        return;
                    }

                    if (ym_first_packet) {
                        if (!YM_ParseFileNamePacket(ym_packet_buf, total_len, &ym_file_size)) {
                            YM_SendCancel();
                            ym_state = YM_STATE_CANCEL;
                            return;
                        }
                        ym_first_packet = 0;
                        Flash_Init();
                        Flash_EraseSector(APP2_ADDR);
                        ym_received = 0;
                        YM_SendAck();
                    } else {
                        if (!YM_WriteDataPacket(ym_packet_buf, data_len)) {
                            YM_SendCancel();
                            ym_state = YM_STATE_CANCEL;
                            return;
                        }
                        YM_SendAck();
                    }
                    ym_expect_seq++;
                    ym_packet_len = 0;
                }
            }
            break;

        default:
            break;
    }
}

/* 启动YModem升级（阻塞等待完成）*/
uint8_t YModem_StartUpgrade(void) {
    uint32_t timeout = 0;
    ym_state = YM_STATE_WAIT_START;
    ym_file_size = 0;
    ym_received = 0;

    while (ym_state != YM_STATE_COMPLETE && ym_state != YM_STATE_CANCEL) {
        Delay_Ms(10);
        timeout++;
        if (timeout > 3000) {  // 30秒超时
            return 0;
        }
    }
    if (ym_state == YM_STATE_COMPLETE && ym_file_size == ym_received) {
        Flash_DeInit();
        return 1;
    }
    return 0;
}

uint8_t YModem_IsComplete(void) {
    return (ym_state == YM_STATE_COMPLETE);
}

uint32_t YModem_GetFileSize(void) {
    return ym_file_size;
}


