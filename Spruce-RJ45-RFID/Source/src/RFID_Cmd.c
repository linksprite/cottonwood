					   

/*************************************************************************************************************
 BAUD RATE: 115200
 CHECK_BIT: NONE
 NUM_BIT  : 8
 STOP_BIT : 1
 CMD1 Send: 10 03 01   Receive: 0X11_AS3991 ROGER Reader Hardware 1.2
 CMD2 Send: 10 03 00   Receive:0X11_AS3991 Mini Reader Firmware 1.5.1
 CMD3 Send:43 04 01 cd  Receive£º44 16 01 ff 54 3a 0d 0e 30 00 30 34 00 00 00 00 03 c0 00 00 3f   17£»
 30 34 00 00 00 00 03 c0 00 00 3f 17 (The receive data is a example,different tags have different serial number)

**************************************************************************************************************/



#include"RFID_Cmd.h"

void delay_(void)
{
	unsigned int count;
	for (count = 10000; count > 0; count--);
}

void cmd_delay(void)
{
	unsigned int count_s;
	for (count_s = 200; count_s > 0; count_s--)
	delay_();
}


void SendCmd1(void)
{

	USART_SendData(USART2, 0x10);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/
	}
	USART_SendData(USART2, 0x03);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/
	}
	USART_SendData(USART2, 0x01);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/
	} 
	 cmd_delay();	//waiting receive data
}

void SendCmd2(void)
{
	
	USART_SendData(USART2, 0x10);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/

	}
	USART_SendData(USART2, 0x03);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/

	}
	USART_SendData(USART2, 0x00);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/
	}	
	cmd_delay(); //waiting receive data

}

void SendCmd3(void)
{
	
	USART_SendData(USART2, 0x43);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/

	}
	USART_SendData(USART2, 0x04);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/

	}
	USART_SendData(USART2, 0x01);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/
	}
	USART_SendData(USART2, 0xcd);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {/* Wait until a byte is to send*/
	}  
	cmd_delay();	//waiting receive data
}




void SendCmd(uint8_t *cmd)
{
  int i;
  for(i=0;i<sizeof(cmd);i++)
  {
	USART_SendData(USART2, cmd[i]);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET){}
  }


}











