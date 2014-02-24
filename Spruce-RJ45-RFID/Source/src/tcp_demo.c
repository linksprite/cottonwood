
#include"RFID_Cmd.h"
#include "tcp_demo.h"
#include "uip.h"
#include <string.h>
#include <stdio.h>
#include "stm32f10x.h"
#include <assert.h> 
#include"stm32f10x_conf.h"

static void aborted(void);
static void timedout(void);
static void closed(void);
static void connected(void);
static void newdata(void);
static void acked(void);
static void senddata(void);

static uint8_t test_data[2048];   /* 1K测试数据 */

#define LED1_ON()	GPIO_SetBits(GPIOB,  GPIO_Pin_5);
#define LED1_OFF()	GPIO_ResetBits(GPIOB,  GPIO_Pin_5);

extern uint8_t ReceiveNum[40];
extern uint8_t R_count;
extern uint16_t F_count;

/*******************************************************************************
*	函数名: tcp_demo_appcall
*	输  入: 无
*	输  出: 无
*	功能说明：这是一个回调函数。在h文件中，我们定义 UIP_APPCALL的宏等于 tcp_demo_appcall
*		当uip事件发生时，UIP_APPCALL 函数会被调用。
*		例如 : 当一个TCP连接被创建时、有新的数据到达、数据已经被应答、数据需要重发等事件
*

*/
void tcp_demo_appcall(void)
{
	if (uip_aborted())
	{
	//	printf("uip_aborted!\r\n");
		aborted();
	}

	if (uip_timedout())
	{
	//	printf("uip_timedout!\r\n");
		timedout();
	}

	if (uip_closed())
	{
	//	printf("uip_closed!\r\n");
		closed();
	}

	if (uip_connected())
	{
	//	printf("uip_connected!\r\n");
		connected();
	}

	if (uip_acked())
	{
		acked();
	}

	/* 接收到一个新的TCP数据包，准备需要发送数据 */
	if (uip_newdata())
	{
		newdata();
	}

	/* 当需要重发、新数据到达、数据包送达、连接建立时，通知uip发送数据 */
	if (uip_rexmit() ||	uip_newdata() || uip_acked() ||	uip_connected() || uip_poll())
	{
		senddata();
	}
}

/*******************************************************************************
*	函数名: aborted
*	输  入: 无
*	输  出: 无
*	功能说明：当TCP连接异常终止时，调用此函数。
*/
static void aborted(void)
{
	;
}

static void timedout(void)
{
	;
}

static void closed(void)
{
	;
}

/*******************************************************************************
*	函数名: connected
*	输  入: 无
*	输  出: 无
*	功能说明：当TCP连接建立时，调用此函数。
*/
static void connected(void)
{   unsigned char a[2]; 
	/*
		uip_conn结构体有一个"appstate"字段指向应用程序自定义的结构体。
		声明一个s指针，是为了便于使用。

		不需要再单独为每个uip_conn分配内存，这个已经在uip中分配好了。
		在uip.c 中 的相关代码如下：
			struct uip_conn *uip_conn;
			struct uip_conn uip_conns[UIP_CONNS]; //UIP_CONNS缺省=10
		定义了1个连接的数组，支持同时创建几个连接。
		uip_conn是一个全局的指针，指向当前的tcp或udp连接。
	*/
	struct tcp_demo_appstate *s = (struct tcp_demo_appstate *)&uip_conn->appstate;

	//memset(test_data, 0x55, 2048);

	s->state = STATE_CMD;
	s->textlen = 0;

	s->textptr = "Great! Connect success!\n";
	s->textlen =strlen((char *)s->textptr);
	
	//senddata();
	
	//a[0]=0x0d; a[1]=0x0a;
	//s->textptr = a;
	//s->textlen = 2;

}


/*******************************************************************************
*	函数名: TCPCmd
*	输  入: 无
*	输  出: 无
*	功能说明：分析PC机网络软件发送的命令，并进行相应的处理。
*/
void TCP_Cmd(struct tcp_demo_appstate *s)
{
	uint8_t Cmd[50],i;
	memcpy(Cmd, uip_appdata, uip_len);//将接受到命令保存在cmd数组中
	for(i=0;i<uip_len;i++)	//通过usart2 发送给RFID
 {
	  USART_SendData(USART2, Cmd[i]);
 	  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) { }
  }
			 
	cmd_delay();	 //等待接受数据
	s->textptr =ReceiveNum;	//返回数据
	s->textlen =F_count;	  //返回数据长度
	R_count=0; //清零 重新计算
	F_count=0;//清零 重新计算

/*	uint8_t cmd; 
	if ((uip_len == 5) && (memcmp("cmd ", uip_appdata, 4) == 0))
	{
		cmd = ((uint8_t *)uip_appdata)[4]; 
		switch(cmd)
		{
		   case '1': SendCmd1(); break;
		   case '2': SendCmd2(); break;
		   case '3': SendCmd3(); break;
		   default : break;
		          
		}
			s->textptr =ReceiveNum;
			s->textlen =F_count;
			R_count=0;
			F_count=0;
	}

	else
	{
	  s->textptr = "Unknow Command!\r\n";
      s->textlen = strlen((char *)s->textptr);
	}*/	
	
						   
}

/*******************************************************************************
*	函数名: newdata
*	输  入: 无
*	输  出: 无
*	功能说明：当收到新的TCP包时，调用此函数。准备数据，但是暂时不发送。
*/
static void newdata(void)
{
	struct tcp_demo_appstate *s = (struct tcp_demo_appstate *)&uip_conn->appstate;
	
	if (s->state == STATE_CMD)
	{
	//	printf("uip_newdata!\r\n");
		TCP_Cmd(s);
	}
	else if (s->state == STATE_TX_TEST)	/* 上传测试状态 */
	{
		/* 在发送测试状态，如果收到PC机发送的任意数据，则退出测试状态 */
		if ((uip_len == 1) && (((uint8_t *)uip_appdata)[0] == 'A'))
		{
			;/* 继续测试 */
		}
		else
		{
			/* 退到命令状态 */
	   		s->state = STATE_CMD;
			s->textlen = 0;
		}
	}
	else if (s->state == STATE_RX_TEST)	/* 下传测试状态 */
	{				
		if ((uip_len == 4) && (memcmp("stop", uip_appdata, 4) == 0))
		{
			/* 退到命令状态 */
	   		s->state = STATE_CMD;
			s->textlen = 0;
		}
		else
		{
			static int sLen;

			sLen = uip_len;
			s->textptr = (uint8_t *)&sLen;		/* 配合PC机测试软件，应答接收到的数据长度 */
			s->textlen = 4;
		}
	}
}

/*******************************************************************************
*	函数名: acked
*	输  入: 无
*	输  出: 无
*	功能说明：当发送的TCP包成功送达时，调用此函数。
*/
static void acked(void)
{
	struct tcp_demo_appstate *s = (struct tcp_demo_appstate *)&uip_conn->appstate;

	switch(s->state)
	{
		case STATE_CMD:		 /* 在命令状态 */
			s->textlen = 0;

			/* 
				只在命令状态打印调试信息 
				避免发送测试时，影响通信速度		
			*/
		//printf("uip_acked!\r\n");
			break;

		case STATE_TX_TEST:
			s->textptr = test_data;	/* 连续发送 */
			s->textlen = 1400;
			break;

		case STATE_RX_TEST:
			s->textlen = 0;
			break;
	}
}	 					

/*******************************************************************************
*	函数名: senddata
*	输  入: 无
*	输  出: 无
*	功能说明：发送tcp数据。
*/
static void senddata(void)
{
	struct tcp_demo_appstate *s = (struct tcp_demo_appstate *)&uip_conn->appstate;

	if (s->textlen > 0)
	{
		/*
			这个函数将向网络发送TCP数据包,
				s->textptr : 发送的数据包缓冲区指针
				s->textlen ：数据包的大小（单位字节）
		*/
		uip_send(s->textptr, s->textlen);
	}
}

/*******************************************************************************
*	函数名: uip_log
*	输  入: m: 字符串
*	输  出: 无
*	功能说明：uIP的调试打印语句。
*		可以通过注释 #define UIP_CONF_LOGGING     而关闭调试输出
*/
void uip_log(char *m)
{
	printf("uIP log message: %s\r\n", m);
}

/*---------------------------------------------------------------------------*/
