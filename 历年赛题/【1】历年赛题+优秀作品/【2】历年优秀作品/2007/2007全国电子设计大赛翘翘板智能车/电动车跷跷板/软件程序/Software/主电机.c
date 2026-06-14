#include<msp430x14x.h>
#include"主电机.h"
#define DIRA BIT3
#define DIRB BIT4
#define GoAhead {P4OUT|=DIRA;P4OUT&=~DIRB;}
#define GoBack {P4OUT&=~DIRA;P4OUT|=DIRB;}
/*-------------------------------------------
  函数名：set_speed(int speed)
  函数功能：设定速度  
  说明：  SPEED>0  正走
          SPEED<0  倒走
--------------------------------------------*/
void set_speed(int speed)
{
  if(speed<0) 
   {speed=-speed; GoBack;}
  else 
   GoAhead;
  TBCCR1=CLK*speed/(FREQ*MAXSPEED);
}
/*-------------------------------------------
  函数名：MMTR_INIT()
  函数功能：主电机初始化  
--------------------------------------------*/
void MMTR_INIT()
{
 TBCTL=TBSSEL_2+ID_0+MC_0;     //时钟选择：SMCLK，不分频（），不允许中断，不启动TB
 TBCCTL1=OUTMOD_7;             //比较模式七输出，不允许中断
 TBCCR0=CLK/FREQ;             
 
 set_speed(0);                 //设定初始速度为0 
 
 P4OUT&=~BIT1;                 //配置P4.1为PWM输出
 P4DIR|=BIT1;                  
 P4SEL|=BIT1;                
 
 P4OUT&=~(DIRA+DIRB);          //配置P4.2/P4.3为方向控制IO
 P4DIR|=DIRA+DIRB;                    
}
/*-------------------------------------------
  函数名：MMTR_START()
  函数功能：开启TB
  说明：主电机开始工作
--------------------------------------------*/
void MMTR_START()
{
 TBCTL|=MC_1;                  // TB 以 UP 方式开始计数
}
/*-------------------------------------------
  函数名：MMTR_STOP()
  函数功能：刹车
--------------------------------------------*/
void MMTR_STOP()
{
 P4OUT&=~(DIRA+DIRB);
}



