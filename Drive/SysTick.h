#ifndef __SYSTICK_H
#define __SYSTICK_H

#include "stm32f4xx.h"

typedef struct {
    uint32_t end_time;
}simple_delay_t;


void delay_ms_nonblocking_start(simple_delay_t* delay, uint32_t ms);
uint8_t delay_ms_nonblocking_check(simple_delay_t* delay);

void SysTick_Init(void);
void Tick_ms(__IO u32 nTime);
void Delay_ms(__IO u32 nTime);
void Delay_us(__IO u32 nTime);

uint32_t GetSystemTick(void);
#endif /* __SYSTICK_H */
