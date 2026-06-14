#include<msp430x14x.h>
#include"舵机.h"

#define MID 1580                //中间 0度
#define LMAX 1950               //最左 


void Set_Angle(int angle)       //设定舵机角度-5~+5
{
   CCR2=MID-(LMAX-MID)*angle/MAXANGL;     //#define MAXANGL 5 
}

void PWM_SM_INIT()
{
 TACTL=TASSEL_2+ID_0+MC_0;      //时钟选择：smclk，不分频（），不允许中断，不启动TA
 TACCTL2=OUTMOD_7;              //比较模式七输出，不允许中断
 TACCR0=20000;                  //20ms为周期
 Set_Angle(0);                  //初始角度为0  
 P1DIR|=BIT3;
 P1SEL|=BIT3;
}

void TA_START()
{
 TACTL|=MC_1;                   // TA 以 UP 方式开始计数
}

void TA_STOP()
{
 TACTL&=~MC_1;
}


