/**
  * @file    jump_app.h
  * @brief   跳转到应用程序、系统复位接口
  * @author  OTA Template
  * @version 1.0
  */

#ifndef __JUMP_APP_H
#define __JUMP_APP_H

#include "stm32f4xx.h"

/**
 * @brief 跳转到指定地址的应用程序
 * @param app_addr 应用程序的起始地址（例如 APP1_ADDR = 0x08009000）
 * @note  跳转前会关闭所有中断、复位外设、设置向量表偏移，然后重置栈指针并跳转
 */
void JumpToApp(uint32_t app_addr);

/**
 * @brief 系统软件复位
 * @note  调用NVIC系统复位，使芯片重启
 */
void SystemReset(void);

#endif /* __JUMP_APP_H */

