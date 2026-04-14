/**
  * @file    delay.c
  */

#include "delay.h"

static uint32_t fac_us = 0;  // 微秒乘数
static uint32_t fac_ms = 0;  // 毫秒乘数

void Delay_Init(void) {
    /* 配置SysTick：时钟源为HCLK/8 (STM32F4默认) */
    uint32_t sysclk = SystemCoreClock;  // 例如168MHz
    fac_us = sysclk / 8000000;          // 每微秒需要的时钟数（168M/8=21M，21个时钟/us）
    fac_ms = (uint16_t)fac_us * 1000;   // 每毫秒需要的时钟数
}

void Delay_Us(uint32_t nus) {
    uint32_t ticks = 0;
    uint32_t told = 0;
    uint32_t tnow = 0;
    uint32_t tcnt = 0;
    uint32_t reload = 0;
    reload = SysTick->LOAD;
    ticks = nus * fac_us;
    told = SysTick->VAL;
    while (1) {
        tnow = SysTick->VAL;
        if (tnow != told) {
            if (tnow < told) tcnt += told - tnow;
            else tcnt += reload - tnow + told;
            told = tnow;
            if (tcnt >= ticks) break;
        }
    }
}

void Delay_Ms(uint32_t nms) {
    while (nms--) {
        Delay_Us(1000);
    }
}

