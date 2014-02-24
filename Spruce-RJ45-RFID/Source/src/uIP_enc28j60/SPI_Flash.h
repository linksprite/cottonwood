#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_  1
//#include "stm32f10x_lib.h"

#define FLASH_CHREAD 0x0B
#define FLASH_CLREAD 0x03
#define FLASH_PREAD	0xD2

#define FLASH_BUFWRITE1 0x84
#define FLASH_IDREAD 0x9F
#define FLASH_STATUS 0xD7
#define PAGE_ERASE 0x81
#define PAGE_READ 0xD2
#define MM_PAGE_TO_B1_XFER 0x53				// 将主存储器的指定页数据加载到第一缓冲区
#define BUFFER_2_WRITE 0x87					// 写入第二缓冲区
#define B2_TO_MM_PAGE_PROG_WITH_ERASE 0x86	// 将第二缓冲区的数据写入主存储器（擦除模式）

#define Dummy_Byte 0xA5

/* Select SPI FLASH: ChipSelect pin low  */
#define Select_Flash()     GPIO_ResetBits(GPIOB, GPIO_Pin_12)
/* Deselect SPI FLASH: ChipSelect pin high */
#define NotSelect_Flash()    GPIO_SetBits(GPIOB, GPIO_Pin_12)



void SPI_Flash_Init(void);	         //SPI初始化
u8 SPI_Flash_ReadByte(void);		//flash操作基本函数，读一个字节
u8 SPI_Flash_SendByte(u8 byte);		//	FLASH操作基本函数，发送一个字节
void FlashPageEarse(u16 page);	//擦除指定的页,页范围0-4095

void FlashPageRead(u16 page,u8 *Data);		//读取整页，页范围0-4095
void FlashPageWrite(u16 page,u8 *Data);		//写一整页，页范围0-4095


void FlashWaitBusy(void);			    //Flash忙检测
void FlashReadID(u8 *ProdustID);		//读取flashID四个字节
	





#endif


