/**
  * @file    crc32.h
  * @brief   CRC32计算（多项式0xEDB88320，初始值0xFFFFFFFF，结果异或0xFFFFFFFF）
  */

#ifndef __CRC32_H
#define __CRC32_H

#include <stdint.h>

/**
 * @brief 计算内存区域的CRC32
 * @param data 数据指针
 * @param len  数据长度（字节）
 * @return CRC32值
 */
uint32_t CRC32_Calculate(const uint8_t *data, uint32_t len);

/**
 * @brief 计算Flash区域的CRC32（按字读取，速度快）
 * @param start_addr Flash起始地址
 * @param len        长度（字节）
 * @return CRC32值
 */
uint32_t CRC32_Flash(uint32_t start_addr, uint32_t len);

#endif /* __CRC32_H */

