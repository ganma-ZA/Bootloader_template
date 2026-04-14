/**
  * @file    usart_driver.h
  * @brief   串口1/2/3基本驱动（支持中断接收）
  */

#ifndef __USART_DRIVER_H
#define __USART_DRIVER_H

#include "stm32f4xx.h"
#include <stdint.h>

/* 选择要使用的串口，可在具体使用时分别初始化 */
void USART1_Init(uint32_t baudrate);
void USART2_Init(uint32_t baudrate);
void USART3_Init(uint32_t baudrate);

/* 发送一个字节 */
void USART_SendByte(USART_TypeDef* USARTx, uint8_t ch);

/* 发送字符串 */
void USART_SendString(USART_TypeDef* USARTx, char* str);

/* 接收回调函数（在中断中调用，用户可重定义）*/
typedef void (*USART_ReceiveCallback)(USART_TypeDef* USARTx, uint8_t data);
void USART_SetReceiveCallback(USART_TypeDef* USARTx, USART_ReceiveCallback callback);

#endif

