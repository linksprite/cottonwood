#include "stm32f10x.h"
GPIO_InitTypeDef GPIO_InitStructure;
#define LED1_ON  GPIO_SetBits(GPIOB, GPIO_Pin_5);	             
#define LED2_ON  GPIO_SetBits(GPIOD, GPIO_Pin_6);
#define LED3_ON  GPIO_SetBits(GPIOD, GPIO_Pin_3);
#define LED1_OFF  GPIO_ResetBits(GPIOB, GPIO_Pin_5);  
#define LED2_OFF  GPIO_ResetBits(GPIOD, GPIO_Pin_6);  
#define LED3_OFF  GPIO_ResetBits(GPIOD, GPIO_Pin_3);  