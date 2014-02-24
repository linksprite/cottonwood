

#define Fix_Note_Address

#include "stm32f10x.h"
#include  "uip.h"

#include  "enc28j60.h"


#define   NET_BASE_ADDR		0x6C100000
#define   NET_REG_ADDR			(*((volatile uint16_t *) NET_BASE_ADDR))
#define   NET_REG_DATA			(*((volatile uint16_t *) (NET_BASE_ADDR + 8)))

//#define FifoPointCheck

/*=============================================================================
  系统全域的变量
  =============================================================================*/
#define ETH_ADDR_LEN			6

static unsigned char mymac[6] = {0x04,0x02,0x35,0x00,0x00,0x01};
static unsigned char myip[4] = {192,168,0,15};
// base url (you can put a DNS name instead of an IP addr. if you have
// a DNS server (baseurl must end in "/"):
static char baseurl[]="http://192.168.0.15/";
static unsigned int mywwwport =80; // listen port for tcp/www (max range 1-254)
// or on a different port:
//static char baseurl[]="http://10.0.0.24:88/";
//static unsigned int mywwwport =88; // listen port for tcp/www (max range 1-254)
//
static unsigned int myudpport =1200; // listen port for udp




/*******************************************************************************
*	函数名: etherdev_init
*	参  数: 无
*	返  回: 无
*	功  能: uIP 接口函数,初始化网卡
*/
void etherdev_init(void)
{	 u8 i;

	/*initialize enc28j60*/
	enc28j60Init(mymac);					  
	//把IP地址和MAC地址写入各自的缓存区	ipaddr[] macaddr[]
	init_ip_arp_udp_tcp(mymac,myip,mywwwport);
	for (i = 0; i < 6; i++)
	{
		uip_ethaddr.addr[i] = mymac[i];
	}
    //指示灯状态:0x476 is PHLCON LEDA(绿)=links status, LEDB(红)=receive/transmit
    //enc28j60PhyWrite(PHLCON,0x7a4);
	//PHLCON：PHY 模块LED 控制寄存器	
	enc28j60PhyWrite(PHLCON,0x0476);	
	enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz

}

/*******************************************************************************
*	函数名: etherdev_send
*	参  数: p_char : 数据缓冲区
*			length : 数据长度
*	返  回: 无
*	功  能: uIP 接口函数,发送一包数据
*/
void etherdev_send(uint8_t *p_char, uint16_t length)
{
	enc28j60PacketSend(length,p_char);

}

uint16_t etherdev_read(uint8_t *p_char)
{
	return enc28j60PacketReceive(1500,p_char);
}
////////////////////////////////////////////////////
void tcp_server_appcall(void){		
	switch(uip_conn->lport) 
	{
		case HTONS(80):
			httpd_appcall(); 
			break;
		case HTONS(1200):
		    tcp_demo_appcall(); 
			break;
	}
}


/*******************************************************************************
*	函数名: etherdev_poll
*	参  数: 无
*	返  回: 无
*	功  能: uIP 接口函数, 采用查询方式接收一个IP包
*/
/*
                              etherdev_poll()

    This function will read an entire IP packet into the uip_buf.
    If it must wait for more than 0.5 seconds, it will return with
    the return value 0. Otherwise, when a full packet has been read
    into the uip_buf buffer, the length of the packet is returned.
*/
uint16_t etherdev_poll(void)
{
	uint16_t bytes_read = 0;
#if 0

	/* tick_count threshold should be 12 for 0.5 sec bail-out
		One second (24) worked better for me, but socket recycling
		is then slower. I set UIP_TIME_WAIT_TIMEOUT 60 in uipopt.h
		to counter this. Retransmission timing etc. is affected also. */
	while ((!(bytes_read = etherdev_read())) && (timer0_tick() < 12)) continue;

	timer0_reset();

#endif
	return bytes_read;
}
