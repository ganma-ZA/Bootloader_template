/**
  * @file    delay.h
  * @brief   基于SysTick的毫秒/微秒延时
  */

#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f4xx.h"

void Delay_Init(void);          // 初始化SysTick，系统时钟需已配置
void Delay_Us(uint32_t nus);    // 微秒延时
void Delay_Ms(uint32_t nms);    // 毫秒延时

#endif

