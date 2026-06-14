#include "sys.h"
#include "uart1.h"
#include "delay.h"
#include "LED.h"
#include "key.h"
#include "pwm.h"
#include "ws2812b.h"
#include "nrf24L01.h"
#include "can.h"
#include "oled.h"
extern uint8_t key_count;
char buf[20];
uint8_t Receive[32]={0};

int main(void)
{
    HAL_Init();                    
    stm32_clock_init(RCC_PLL_MUL9);
	
	lde_init();
	key_init();
	dma_init();
//	oled_init();
	NRF24L01_Init();
	pwm_init(90-1,0);
    uart1_init(115200);

	printf("running...\r\n");
	
	nrf2401_chack();	
	lde2_on();

	WS2812_SetALL(255,0,0);
	WS2812_Updata();
	delay_ms(10);
	
    while(1)
    {


	key_scan();
     if(NRF24L01_RxPacket(Receive)==0){}

//		if((Receive[1] & 0x08) != 0x08){
//			key_count = 0;
//			WS2812_SetALL(255,0,0);
//			WS2812_Updata();
//			delay_ms(5);	
//						
//		}else{
//		 	WS2812_Set(key_count/3,0,0,0);
//			WS2812_Updata(); 
//			delay_ms(5);
//		}
		
		
/************************遥控换色******************/		
		if((Receive[1] & 0x08) != 0x08){
			WS2812_SetALL(255,0,0);
			WS2812_Updata();
			delay_ms(40);
		}else{
			WS2812_SetALL(0,0,255);
			WS2812_Updata();
			delay_ms(40);
		}
///************************受击灯效******************/
//	if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_15) == GPIO_PIN_SET)
//	{
//	
//			WS2812_SetALL(0,0,0);
//			WS2812_Updata();
//			delay_ms(80);
//		
//			WS2812_SetALL(255,0,0);
//			WS2812_Updata();
//			delay_ms(40);

//			WS2812_SetALL(0,0,0);
//			WS2812_Updata();
//			delay_ms(80);		
//	}
/************************变速流水灯******************/
//WS2812_FlowLight(255,0,0,100-Receive[2],0);

/************************击打检测******************/
//			key_scan();
//			sprintf(buf, "num:%d", key_count); 
//			oled_show_string(0, 0, buf, 16);      // 在第2页第0列显示数值

//			WS2812_Set(key_count/3,0,0,0);
//			WS2812_Updata(); 
//			delay_ms(5);






//		WS2812_SetALL(255,0,0);
//		WS2812_Updata();
//		delay_ms(10);
	
//		WS2812_FlowLight(255,0,0,80,0);	
//		for(int i=0;i<30;i++){
//			printf("%x,",Receive[i]);
//		}printf("\r\n");
//		delay_ms(10);
	}
}
/*		
1.ws2812测试	
	
		WS2812_SetALL(255,0,0);
		WS2812_Updata();
		delay_ms(1000);
		WS2812_SetALL(0,255,0);
		WS2812_Updata();
		delay_ms(1000);			
		WS2812_SetALL(0,0,255);
		WS2812_Updata();
		delay_ms(1000);	
2.掉血测试
		
		if(key_edge_detect() == true){

		key_num++; // 每检测到一次下降沿，count加1
		
		if(key_num<10){
			WS2812_SetALL(0,0,255);
			WS2812_Updata();
			delay_ms(10);
			
		}else{
			WS2812_SetALL(0,0,0);
			WS2812_Updata();
			delay_ms(10);
		}
    }
	if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_12) == GPIO_PIN_RESET){ 
			key_num  = 0;
			WS2812_SetALL(0,0,255);
			WS2812_Updata();
			delay_ms(10);
	}
3.NRF24L01无线通讯
初始化
	 while(NRF24L01_Check()){
			printf("硬件查寻不到NRF24L01无线模块\n"); 
			delay_ms(1000);
		}
			printf("NRF24L01无线模块硬件连接正常\r\n");

	
		NRF24L01_TX_Mode();//设置为发送模式
		printf("进入数据发送模式，每1s发送一次数据\r\n");
		
		NRF24L01_RX_Mode();
		printf("进入数据接收模式\n");	
循环
			//发送端   
		 if(NRF24L01_TxPacket(Send_Out)==TX_OK){
				printf("NRF24L01无线模块数据发送成功：%s\r\n",Send_Out);
			}else{
				printf("NRF24L01无线模块数据发送失败\r\n");
			} 

			//接收端
      if(NRF24L01_RxPacket(Receive)==0){
      Receive[32]=0;//加入字符串结束符      
      printf("NRF24L01无线模块数据接收成功：%s\r\n",Receive);
    }	
	
	*/

