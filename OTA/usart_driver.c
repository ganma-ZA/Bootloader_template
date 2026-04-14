/**
  * @file    usart_driver.c
  */
#include "string.h"
#include "usart_driver.h"

/* 为每个串口保存回调函数 */
static USART_ReceiveCallback usart1_cb = NULL;
static USART_ReceiveCallback usart2_cb = NULL;
static USART_ReceiveCallback usart3_cb = NULL;

/* 通用初始化函数（GPIO、时钟、NVIC）*/
static void USART_CommonInit(USART_TypeDef* USARTx, uint32_t baudrate, uint32_t pin_tx, uint32_t pin_rx, GPIO_TypeDef* gpio_port, uint32_t gpio_clk, uint32_t usart_clk, IRQn_Type irq) {
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    /* 使能时钟 */
    RCC_AHB1PeriphResetCmd(gpio_clk, ENABLE);
    RCC_APB2PeriphClockCmd(usart_clk, ENABLE);

    /* 配置TX和RX引脚（复用推挽）*/
    GPIO_InitStruct.GPIO_Pin = pin_tx | pin_rx;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(gpio_port, &GPIO_InitStruct);

    /* 连接复用功能（根据实际引脚查数据手册）*/
    if (USARTx == USART1) {
        GPIO_PinAFConfig(gpio_port, GPIO_PinSource9, GPIO_AF_USART1);
        GPIO_PinAFConfig(gpio_port, GPIO_PinSource10, GPIO_AF_USART1);
    } else if (USARTx == USART2) {
        GPIO_PinAFConfig(gpio_port, GPIO_PinSource2, GPIO_AF_USART2);
        GPIO_PinAFConfig(gpio_port, GPIO_PinSource3, GPIO_AF_USART2);
    } else if (USARTx == USART3) {
        GPIO_PinAFConfig(gpio_port, GPIO_PinSource10, GPIO_AF_USART3);
        GPIO_PinAFConfig(gpio_port, GPIO_PinSource11, GPIO_AF_USART3);
    }

    /* 配置USART */
    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USARTx, &USART_InitStruct);

    /* 使能接收中断 */
    USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
    NVIC_InitStruct.NVIC_IRQChannel = irq;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    USART_Cmd(USARTx, ENABLE);
}

void USART1_Init(uint32_t baudrate) {
    USART_CommonInit(USART1, baudrate, GPIO_Pin_9, GPIO_Pin_10, GPIOA, RCC_AHB1Periph_GPIOA, RCC_APB2Periph_USART1, USART1_IRQn);
}

void USART2_Init(uint32_t baudrate) {
    USART_CommonInit(USART2, baudrate, GPIO_Pin_2, GPIO_Pin_3, GPIOA, RCC_AHB1Periph_GPIOA, RCC_APB1Periph_USART2, USART2_IRQn);
}

void USART3_Init(uint32_t baudrate) {
    USART_CommonInit(USART3, baudrate, GPIO_Pin_10, GPIO_Pin_11, GPIOB, RCC_AHB1Periph_GPIOB, RCC_APB1Periph_USART3, USART3_IRQn);
}

void USART_SendByte(USART_TypeDef* USARTx, uint8_t ch) {
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
    USART_SendData(USARTx, ch);
}

void USART_SendString(USART_TypeDef* USARTx, char* str) {
    while (*str) {
        USART_SendByte(USARTx, (uint8_t)(*str++));
    }
}

void USART_SetReceiveCallback(USART_TypeDef* USARTx, USART_ReceiveCallback callback) {
    if (USARTx == USART1) usart1_cb = callback;
    else if (USARTx == USART2) usart2_cb = callback;
    else if (USARTx == USART3) usart3_cb = callback;
}

/* 中断服务函数（需要在工程中实现，这里给出框架）*/
void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART1);
        if (usart1_cb) usart1_cb(USART1, data);
        /* 也可以直接在这里调用YModem或蓝牙的数据处理函数 */
    }
}

void USART2_IRQHandler(void) {
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART2);
        if (usart2_cb) usart2_cb(USART2, data);
    }
}

void USART3_IRQHandler(void) {
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART3);
        if (usart3_cb) usart3_cb(USART3, data);
    }
}

