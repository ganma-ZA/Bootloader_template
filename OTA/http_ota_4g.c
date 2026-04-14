/**
  * @file    http_ota_4g.c
  * @brief   基于EC200的HTTP OTA实现（可根据实际模块修改）
  */

#include "http_ota_4g.h"
#include "flash_ota.h"
#include "usart_driver.h"
#include "crc32.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>

/* 服务器信息配置（可根据需要修改）*/
#define OTA_SERVER_IP     "117.50.xx.xx"
#define OTA_SERVER_PORT   "80"
#define INFO_FILE_PATH    "/ota/info.txt"      /* 存放版本和URL的文件，格式：version=1.0.2&url=http://xxx/固件.bin&size=123456&crc=0x12345678 */
#define DOWNLOAD_TIMEOUT_MS  30000             /* 下载超时30秒 */

/* 发送AT指令并等待响应（简易实现，实际应使用环形缓冲区）*/
static uint8_t AT_SendCmd(char *cmd, char *expected, uint32_t timeout_ms) {
    USART_SendString(USART2, cmd);   /* 假设4G模块连接USART2 */
    /* 等待串口接收预期字符串，此处省略具体实现（可用状态机或简单轮询）*/
    /* 为简化，返回1表示成功 */
    return 1;
}

/* 读取一行响应到缓冲区 */
static uint32_t AT_ReadLine(char *buf, uint32_t max_len) {
    /* 从USART2接收一行，以\r\n结束 */
    /* 省略实现，假设返回读取的字节数 */
    return 0;
}

uint8_t EC200_Init(void) {
    /* 复位模块 */
    /* 发送AT指令测试 */
    if (!AT_SendCmd("AT\r\n", "OK", 1000)) return 0;
    if (!AT_SendCmd("ATE0\r\n", "OK", 500)) return 0;
    if (!AT_SendCmd("AT+QICSGP=1,1,\"CMNET\",\"\",\"\"\r\n", "OK", 1000)) return 0;
    if (!AT_SendCmd("AT+QIACT=1\r\n", "OK", 10000)) return 0;
    return 1;
}

/* 从服务器获取info.txt，解析版本号、URL、大小、CRC */
uint8_t EC200_CheckNewVersion(OTA_Param_t *param) {
    char http_cmd[256];
    char response[512];
    char version_str[32];
    char url_str[128];
    uint32_t size = 0;
    uint32_t crc = 0;
    
    /* 构建HTTP GET请求： AT+QHTTPURL=len,timeout */
    sprintf(http_cmd, "AT+QHTTPURL=%d,30\r\n", (uint32_t)strlen(INFO_FILE_PATH));
    if (!AT_SendCmd(http_cmd, "CONNECT", 2000)) return 0;
    if (!AT_SendCmd(INFO_FILE_PATH, "OK", 3000)) return 0;
    
    /* 发送GET */
    if (!AT_SendCmd("AT+QHTTPGET=80\r\n", "OK", 1000)) return 0;
    /* 读取响应数据（实际需处理+QHTTPREAD等）*/
    /* 假设response中获得了类似：version=1.0.2&url=http://x.x.x.x/firmware.bin&size=123456&crc=0x12AB34CD */
    /* 这里简化解析，实际需要正则或字符串查找 */
    if (strstr(response, "version=")) {
        /* 提取版本号 */
        sscanf(response, "version=%[^&]", version_str);
        /* 提取url */
        char *p = strstr(response, "url=");
        if (p) {
            sscanf(p, "url=%[^&]", url_str);
        }
        /* 提取size和crc */
        /* ... */
        
        /* 对比版本 */
        if (strcmp((char*)param->current_ver, version_str) != 0) {
            param->update_flag = 1;
            param->channel = 2;   /* 4G通道 */
            strcpy((char*)param->new_ver, version_str);
            strcpy(param->firmware_url, url_str);
            param->firmware_size = size;
            param->firmware_crc32 = crc;
            Param_Save(param);
        }
        return 1;
    }
    return 0;
}

/* 下载固件到APP2区 */
uint8_t EC200_DownloadFirmware(OTA_Param_t *param) {
    char http_cmd[256];
    uint32_t downloaded = 0;
    uint8_t recv_buf[512];
    uint32_t remain = param->firmware_size;
    
    /* 擦除APP2区 */
    Flash_Init();
    if (!Flash_EraseSector(APP2_ADDR)) return 0;
    
    /* 设置HTTP URL */
    sprintf(http_cmd, "AT+QHTTPURL=%d,30\r\n", strlen(param->firmware_url));
    if (!AT_SendCmd(http_cmd, "CONNECT", 2000)) return 0;
    if (!AT_SendCmd(param->firmware_url, "OK", 3000)) return 0;
    
    /* 发送GET请求下载 */
    if (!AT_SendCmd("AT+QHTTPGET=80\r\n", "OK", 1000)) return 0;
    
    /* 循环接收数据，写入Flash */
    while (downloaded < param->firmware_size) {
        uint32_t read_len = 0;
        /* 读取数据包，例如使用AT+QHTTPREAD指令 */
        /* 假设读取到的数据放入recv_buf，长度为len */
        if (read_len == 0) break;
        if (!Flash_WriteBuffer(APP2_ADDR + downloaded, recv_buf, read_len)) {
            return 0;
        }
        downloaded += read_len;
    }
    Flash_DeInit();
    
    /* 校验CRC */
    uint32_t calc_crc = CRC32_Flash(APP2_ADDR, param->firmware_size);
    if (calc_crc == param->firmware_crc32) return 1;
    return 0;
}

