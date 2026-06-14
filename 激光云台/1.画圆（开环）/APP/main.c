#include "board.h"
#include "delay.h"
#include "usart.h"
#include "Emm_V5.h"
#include "math.h"
#include "stdlib.h"

int cx,cy;                           //坐标变量
unsigned char len;	                 //长度变量
unsigned int number = 0;
/**********************************************************
***	Emm_V5.0步进闭环控制例程
***	编写作者：ZHANGDATOU
***	技术支持：张大头闭环伺服
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：http s://blog.csdn.net/zhangdatou666
***	qq交流群：262438510
**********************************************************/

/**
	*	@brief		MAIN函数
	*	@param		无
	*	@retval		无
	*/
	

int main(void)
{
		const  float priod = 500.0;  //周期(毫秒)
  	static uint32_t MoveTimeCnt = 0;
	  unsigned int set_x = 0;
	  unsigned int set_y = 0;
	  unsigned int A = 500;
	  unsigned int B = 1000;
	  float phase = 0.0;
	  float Normalization = 0.0;
	  float Omega = 0.0;
/**********************************************************
***	初始化板载外设
**********************************************************/
	board_init();

/**********************************************************
***	上电延时2秒等待Emm_V5.0闭环初始化完毕
**********************************************************/	
	delay_ms(2000);

/**********************************************************
***	位置模式：方向CW，速度1000RPM，加速度0（不使用加减速直接启动），脉冲数3200（16细分下发送3200个脉冲电机转一圈），相对运动
**********************************************************/	
  //Emm_V5_Pos_Control(1, 0, 1000, 0, 3200, 0, 0);
/**********************************************************
***	WHILE循环
**********************************************************/	
	while(1)
	{
	  MoveTimeCnt += 1;							                //每5ms运算1次
		Normalization = (float)MoveTimeCnt / priod;	  //对周期归一化
	  if(MoveTimeCnt==500)
		MoveTimeCnt = 0;
	  Omega = 2.0*3.14159*Normalization;			      //对2π进行归一化处理   	  
		phase = 3.141592/2.0;		 											//逆时针旋转相位差90° 	
		set_x = 500+A*sin(Omega);			   							//计算出X方向位置
	  set_y = 1000-B*sin(Omega+phase); 	 						//计算出Y方向位置
		Emm_V5_Pos_Control(2, 0, 100, 0, set_x , 1, 0);
		delay_ms(1);
		Emm_V5_Pos_Control(1, 0, 100, 0, set_y , 1, 0);
		delay_ms(100);		
	}
}
