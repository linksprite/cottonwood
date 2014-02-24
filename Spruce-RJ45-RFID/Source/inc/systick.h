/****************************************************************************
* Copyright (C), 2009-2010, www.armfly.com
*
* 文件名: systick.h
* 内容简述: 头文件
*
* 文件历史:
* 版本号  日期       作者    说明
* v0.1    2009-12-27 armfly  创建该文件
*
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SYSTICK_H
#define __SYSTICK_H

/* 等待定时器超时期间，可以让CPU进入IDLE状态， 目前是空 */
#define CPU_IDLE()

/* 定时器结构体，成员变量必须是 volatile, 否则C编译器优化时可能有问题 */
typedef struct
{
	volatile uint32_t count;	/* 计数器 */
	volatile uint8_t flag;		/* 定时到达标志  */
}SOFT_TMR;

void SysTick_Configuration(void);
void DelayMS(uint32_t n);

void StartTimer(uint8_t _id, uint32_t _period);
uint8_t CheckTimer(uint8_t _id);
int32_t GetRunTime(void);

#endif
