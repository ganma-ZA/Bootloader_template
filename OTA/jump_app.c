#include "jump_app.h"
#include "stm32f4xx.h"

/* 关闭所有外设时钟（可选，减少跳转后冲突）*/
static void DeInitAllPeripherals(void) {
    RCC_DeInit();
    /* 可根据需要添加各外设的DeInit，例如 TIM_DeInit(TIM1); */
}

/**
 * @brief 跳转到应用程序
 */
void JumpToApp(uint32_t app_addr) {
    uint32_t msp_value = *(uint32_t*)app_addr;          /* 栈顶指针 */
    uint32_t reset_handler = *(uint32_t*)(app_addr + 4);/* 复位中断函数地址 */

    /* 检查栈顶指针是否在SRAM范围（0x20000000 - 0x20020000）*/
    if (msp_value < 0x20000000 || msp_value >= 0x20020000) {
        return;  /* 无效地址，不跳转 */
    }

    /* 关闭全局中断 */
    __disable_irq();

    /* 关闭SysTick并复位其计数器 */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    /* 复位所有外设（避免中断残留）*/
    DeInitAllPeripherals();

    /* 设置中断向量表偏移 */
    SCB->VTOR = app_addr;

    /* 设置主堆栈指针 */
    __set_MSP(msp_value);

    /* 强制类型转换，得到跳转函数 */
    void (*app_entry)(void) = (void(*)(void))reset_handler;

    /* 重新使能中断 */
    __enable_irq();

    /* 跳转到APP */
    app_entry();
}

/**
 * @brief 系统软复位
 */
void SystemReset(void) {
    __disable_irq();
    NVIC_SystemReset();
}

