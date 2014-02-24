#include "stm32f10x.h"
#include "SPI_Flash.h"
#include "stm32f10x_spi.h"

void SPI_Flash_Init(void);
u8 SPI_Flash_ReadByte(void);
u8 SPI_Flash_SendByte(u8 byte);
void FlashPageEarse(u16 page);
void FlashPageRead(u16 page,u8 *Data);
void FlashPageWrite(u16 page,u8 *Data);
void FlashWaitBusy(void);
void AT45_RomTo_buf(unsigned char buffer,unsigned int page);
u8 AT45_buf_ToRam(unsigned char buffer,unsigned int start_address,unsigned int length);
u8 AT45_RamTo_buf(unsigned char buffer,unsigned int start_address,unsigned int length);	  
void AT45_buf_ToRom(unsigned char buffer,unsigned int page);
void AT45_page_earse(unsigned int page);


unsigned char AT45_buffer[528];

void FlashReadID(u8 *Data)
{
	u8 i;
	Select_Flash();	
  	SPI_Flash_SendByte(0x9F);
  	for(i = 0; i < 4; i++)
  	{
  		Data[i] = SPI_Flash_ReadByte();
  	}
  	NotSelect_Flash();	
}

/*******************************************************************************
* Function Name  : SPI_FLASH_Init
* Description    : Initializes the peripherals used by the SPI FLASH driver.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SPI_Flash_Init(void)
{
  SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
   
  /* Enable SPI2 GPIOB clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2 ,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  /* Configure SPI2 pins: SCK, MISO and MOSI */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Configure PB.12 as Output push-pull, used as Flash Chip select */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Deselect the FLASH: Chip Select high */
  NotSelect_Flash();

  /* SPI2 configuration */ 
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI2, &SPI_InitStructure);
  
  /* Enable SPI2  */
  SPI_Cmd(SPI2, ENABLE);   
}

/*******************************************************************************
* Function Name  : SPI_FLASH_ReadByte
* Description    : Reads a byte from the SPI Flash.
*                  This function must be used only if the Start_Read_Sequence
*                  function has been previously called.
* Input          : None
* Output         : None
* Return         : Byte Read from the SPI Flash.
*******************************************************************************/
u8 SPI_Flash_ReadByte(void)
{
  return (SPI_Flash_SendByte(Dummy_Byte));
}

/*******************************************************************************
* Function Name  : SPI_FLASH_SendByte
* Description    : Sends a byte through the SPI interface and return the byte 
*                  received from the SPI bus.
* Input          : byte : byte to send.
* Output         : None
* Return         : The value of the received byte.
*******************************************************************************/
u8 SPI_Flash_SendByte(u8 byte)
{
  /* Loop while DR register in not emplty */
  while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
  //NotSelect_Flash();  while(1);
  /* Send byte through the SPI2 peripheral */
  SPI_I2S_SendData(SPI2, byte);

  /* Wait to receive a byte */
  while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);

  /* Return the byte read from the SPI bus */
  return SPI_I2S_ReceiveData(SPI2);
}

void FlashPageEarse(u16 page)
{	
	FlashWaitBusy();	
	Select_Flash();
	SPI_Flash_SendByte(PAGE_ERASE);
	SPI_Flash_SendByte((u8)(page >> 6));
	SPI_Flash_SendByte((u8)(page << 2));
	SPI_Flash_SendByte(Dummy_Byte);
	//|-23-|-22-|21|20|19|18|17|16|15|14|13|12|11|10|9|8|7|6|5|4|3|2|1|0|
	//|2个无关位|------------12位页地址-------------|----10个无关位-----|
	NotSelect_Flash();	
}

void FlashPageRead(u16 page,u8 *Data)
{
	u16 i;	
	FlashWaitBusy();
	Select_Flash();
	SPI_Flash_SendByte(PAGE_READ);
	SPI_Flash_SendByte((u8)(page >> 6));
  SPI_Flash_SendByte((u8)(page << 2));
  SPI_Flash_SendByte(0x00);//3个字节
  SPI_Flash_SendByte(0x00);
  SPI_Flash_SendByte(0x00);
  SPI_Flash_SendByte(0x00);
  SPI_Flash_SendByte(0x00);
  for (i = 0;i < 528; i++)
	{
		Data[i] = SPI_Flash_ReadByte();
	}
	NotSelect_Flash();	
}

void FlashPageWrite(u16 page,u8 *Data)		//写一整页，页范围0-4095
{
	unsigned int i;
	FlashWaitBusy();
	Select_Flash();
	SPI_Flash_SendByte(BUFFER_2_WRITE);
	SPI_Flash_SendByte(0x00);
	SPI_Flash_SendByte(0x00);
	SPI_Flash_SendByte(0x00);
	for (i = 0;i < 528; i++)
	{
		SPI_Flash_SendByte(Data[i]);
	}
	NotSelect_Flash();
		
	if ( page < 4096)
	{
		Select_Flash();
		SPI_Flash_SendByte(B2_TO_MM_PAGE_PROG_WITH_ERASE);
		SPI_Flash_SendByte((u8)(page>>6));
		SPI_Flash_SendByte((u8)(page<<2));
		SPI_Flash_SendByte(0x00);
		NotSelect_Flash();
		FlashWaitBusy();
	}	
}

void FlashWaitBusy(void)
{
	unsigned char state_reg=0x00;
	Select_Flash();	
	SPI_Flash_SendByte(FLASH_STATUS);	 
	//while(1)SPI_Flash_SendByte(0x00);
	while((state_reg&0x80) == 0)
	{	//NotSelect_Flash();
		//Select_Flash();	
		state_reg = SPI_Flash_ReadByte();
		//NotSelect_Flash();
	}
	NotSelect_Flash();
}

//将指定主存储器页的数据转入指定缓冲区
void AT45_RomTo_buf(unsigned char buffer,unsigned int page)
{ 
	FlashWaitBusy();
	Select_Flash();
	if (buffer==1)
		SPI_Flash_SendByte(0x53);      //写入第一缓冲区
	else
		SPI_Flash_SendByte(0x55);	   //写入第二缓冲区
	SPI_Flash_SendByte((unsigned char)(page >> 6));
    SPI_Flash_SendByte((unsigned char)(page << 2));
    SPI_Flash_SendByte(0x00);		 	
	NotSelect_Flash();
}

//读取指定缓冲区指定单元的数据，保存在DF_buffer[]数组中
unsigned char AT45_buf_ToRam(unsigned char buffer,unsigned int start_address,unsigned int length)
{
	unsigned int i;
	if ((527-start_address)>=length)
	{
		FlashWaitBusy(); 
		Select_Flash(); 		
		if (buffer==1)
			SPI_Flash_SendByte(0xd4); 	   //读取第一缓冲区
		else
			SPI_Flash_SendByte(0xd6);	   //读取第二缓冲区
		SPI_Flash_SendByte(0x00);                        
		SPI_Flash_SendByte((unsigned char)(start_address >> 8));   
		SPI_Flash_SendByte((unsigned char)start_address);        
		SPI_Flash_SendByte(0x00);
		for (i=0;i<length;i++)
		{
			//SPI_Flash_SendByte(0xFF);
			AT45_buffer[i] = SPI_Flash_ReadByte();
		}
		NotSelect_Flash();
		return 1;
	}
	else
		return 0;
}

//将DF_buffer[]数组中指定长度的数据写入指定缓冲区
unsigned char AT45_RamTo_buf(unsigned char buffer,unsigned int start_address,unsigned int length)
{
	unsigned int i;
	if  ((527-start_address)>=length)
	{
		FlashWaitBusy(); 
		Select_Flash(); 	
		if (buffer==1) SPI_Flash_SendByte(0x84);      //写入第一缓冲区
   		else SPI_Flash_SendByte(0x87);	              //写入第二缓冲区

		SPI_Flash_SendByte(0x00);
		SPI_Flash_SendByte((unsigned char)(start_address >> 8));   
		SPI_Flash_SendByte((unsigned char)start_address);
		for (i=0;i<length;i++)
			SPI_Flash_SendByte(AT45_buffer[i]);
		NotSelect_Flash();
		return 1;
	}
	else
		return 0;
}


//将指定缓冲区中的数据写入主存储区的指定页
void AT45_buf_ToRom(unsigned char buffer,unsigned int page)
{
	FlashWaitBusy(); 
	Select_Flash(); 
	if (page<4096)
	{
		Select_Flash(); 
		if (buffer==1)
			SPI_Flash_SendByte(0x83);	// 将第一缓冲区的数据写入主存储器（擦除模式）
		else
			SPI_Flash_SendByte(0x86);	// 将第二缓冲区的数据写入主存储器（擦除模式）
		SPI_Flash_SendByte((unsigned char)(page>>6));
		SPI_Flash_SendByte((unsigned char)(page<<2));
		SPI_Flash_SendByte(0x00);
		NotSelect_Flash();
	}
	//DF_SPI_OFF;
}

//擦除指定的主存储器页（地址范围0-4095）
void AT45_page_earse(unsigned int page)
{
	FlashWaitBusy(); 
	Select_Flash(); 	
	SPI_Flash_SendByte(0x81);
	SPI_Flash_SendByte((unsigned char)(page >> 6));
	SPI_Flash_SendByte((unsigned char)(page << 2));
	SPI_Flash_SendByte(0x00);
	NotSelect_Flash();
}