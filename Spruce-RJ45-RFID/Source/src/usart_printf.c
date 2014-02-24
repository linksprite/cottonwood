/****************************************************************************

*
* 文件名: usart_printf.c
* 内容简述: 本模块实现printf和scanf函数重定向到串口1
*	实现重定向，只需要添加2个函数
		int fputc(int ch, FILE *f);
		int fgetc(FILE *f);
*


*
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include <stdio.h>

/*******************************************************************************
	函数名：PrintfLogo
	输  入: 例程名称和例程最后更新日期
	输  出:
	功能说明：
*/
void PrintfLogo(char *strName)
{
	//printf("*************************************************************\n\r");
	//printf("* Example Name : %s\r\n", strName);
//	printf("* \n\r");
	//printf("*************************************************************\n\r");
}

/*******************************************************************************
	函数名：USART_Configuration
	输  入:
	输  出:
	功能说明：
	初始化串口硬件设备，未启用中断。
	配置步骤：
	(1)打开GPIO和USART的时钟
	(2)设置USART两个管脚GPIO模式
	(3)配置USART数据格式、波特率等参数
	(4)最后使能USART功能
*/
void USART_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*PA3->USART2 RX GPIO_CONFIGURATION*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*PA2->USART2 TX GPIO_CONFIGURATION*/
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	


	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);
	USART_Init(USART2, &USART_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);


	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//ENABLE USART2 RECEIVE INTRRUPT
	
	
	USART_Cmd(USART1, ENABLE);
	USART_Cmd(USART2, ENABLE);//ENABLE USART2

	USART_ClearFlag(USART1, USART_FLAG_TC);
	USART_ClearFlag(USART2, USART_FLAG_TC);      
}

/*******************************************************************************
	函数名：fputc
	输  入:
	输  出:
	功能说明：
	重定义putc函数，这样可以使用printf函数从串口1打印输出
*/
int fputc(int ch, FILE *f)
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART */
	USART_SendData(USART1, (uint8_t) ch);

	/* Loop until the end of transmission */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{}

	return ch;
}

/*******************************************************************************
	函数名：fputc
	输  入:
	输  出:
	功能说明：
	重定义getc函数，这样可以使用scanff函数从串口1输入数据
*/
int fgetc(FILE *f)
{
	/* 等待串口1输入数据 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET)
	{}

	return (int)USART_ReceiveData(USART1);
}

