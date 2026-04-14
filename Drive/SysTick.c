#include "SysTick.h"

static __IO u32 TimingDelay=0;
static __IO u32 Tickms=0;
static __IO u32 msTick_num=0;


// 系统时钟变量
extern uint32_t system_tick;

/*
 * 函数名：SysTick_Init
 * 描述  ：启动系统滴答定时器 SysTick
 * 输入  ：无
 * 输出  ：无
 * 调用  ：外部调用 
 */
void SysTick_Init(void)
{
	/* SystemFrequency / 1000    1ms中断一次
	 * SystemFrequency / 100000	 10us中断一次
	 * SystemFrequency / 1000000 1us中断一次
	 */
//	if (SysTick_Config(SystemFrequency / 100000))	// ST3.0.0库版本
	if (SysTick_Config(SystemCoreClock / 1000))	// ST3.5.0库版本
	{ 
		/* Capture error */ 
		while (1);
	}
	// 使能滴答定时器  
	SysTick->CTRL |=  SysTick_CTRL_ENABLE_Msk;
}

// 开始非阻塞延时
void delay_ms_nonblocking_start(simple_delay_t* delay, uint32_t ms)
{
    delay->end_time = GetSystemTick() + ms;
}

// 检查非阻塞延时是否完成
uint8_t delay_ms_nonblocking_check(simple_delay_t* delay)
{
    return (GetSystemTick() >= delay->end_time);
}

void Delay_ms(__IO u32 nTime)
{ 
	TimingDelay = nTime;	

	while(TimingDelay != 0);
}

void Delay_us(__IO u32 nTime)
{ 
	TimingDelay = nTime*10;	

	while(TimingDelay != 0);
}

void Tick_ms(__IO u32 nTime)
{ 
	Tickms = nTime;	
}


uint32_t GetSystemTick(void)
{
    return system_tick;
}



