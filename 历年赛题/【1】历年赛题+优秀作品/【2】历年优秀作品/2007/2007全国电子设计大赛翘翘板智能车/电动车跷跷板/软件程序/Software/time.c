#include"global.h"
#include"主电机.h"
#include"msp430x14x.h"
#include"display.h"
#include"舵机.h"

#include"LCD1602.H"


#define BEEP BIT4

unsigned char pre_work_mode;

void time_init()
{
 time_start_to_bal=0;           //上板模式的需要3秒
 time_bal_search=0;
 time_bal_wait=0;
 time_bal_to_end=0;
 time_end_wait=0;
 time_end_to_start=0;
 pre_work_mode=0; 
 P3DIR|=BEEP;
 P3OUT&=~BEEP;
}

void delay()                   //模式切换时蜂鸣器延时
{
 for(unsigned int k=0;k<5000;k++)
   for(unsigned int j=0;j<50;j++);
};

void delay1()                     //模式切换时进行延时
{
   for(unsigned int k=0;k<5000;k++)
   for(unsigned int j=0;j<200;j++);
}

void time_count()                 //各模式计时
{
  
 if(pre_work_mode!=work_mode)     //如果模式改变，停止，蜂鸣器提示
 { 
   MMTR_STOP();
   P3OUT|=BEEP;   
   delay();
   P3OUT&=~BEEP; 
   if((work_mode!=BAL_WAIT)&&(work_mode!=END_WAIT)&&(work_mode!=OVER)&&(work_mode!=BAL_SEARCH)&&(work_mode!=END_TO_START))
   delay1();
 };
 
 pre_work_mode=work_mode;         //用于确认是否经过模式改变
 
 switch(work_mode)
 {   
  case START_TO_BAL:              //从开头到平衡附近      
   time_start_to_bal++;
   set_speed(100);
   if(sys_mode==1)
   {
      if(time_start_to_bal>77)
      {
        work_mode=BAL_SEARCH;
        Set_Angle(0);
      }
   }  
   else if(sys_mode==2)           //因为扩展模式需要前2秒的上板时间，所以延时不同
   {
      if(time_start_to_bal>77)
      {
        work_mode=BAL_SEARCH;
        Set_Angle(0);
        time_start_to_bal+=20;    //补充上2秒上板时间
      }
   }
  break;
   
  case BAL_SEARCH:                //平衡查找   
      time_bal_search++;
      light_on();               //开液晶背光
      if(balance_flag==1)
      {
        time_bal_insure++;
        if(time_bal_insure>30)
        {
          time_bal_insure=0;
          work_mode=BAL_WAIT;
        }
      }
  break;
   
  case BAL_WAIT:                  //平衡后等待
   time_bal_wait++;
   if(balance_flag==0)
     work_mode=BAL_SEARCH;     
   if(sys_mode==1)                //如是基本演示，平衡后延时5秒直接跑到终点
   {
     if(time_bal_wait>=50)
    {
     work_mode=BAL_TO_END;
     light_off();                 //关液晶背光 
    }
   }
   else if(sys_mode==2)           //如是扩展演示，平衡后延时5秒等待放入重物
   {
    if(time_bal_wait>=50)
    {
     time_bal_wait=0;
     work_mode=NEXT_BAL;
     light_off();                 //关液晶背光 
    }
   }   
  break;
   
  case BAL_TO_END:                //从平衡到末尾
   time_bal_to_end++;
   set_speed(100);
  break;
  
  case END_WAIT:                  //末尾等待
   time_end_wait++;
   if(time_end_wait>50)           //超过五秒，返回
   {
    dir_flag=-1;
    work_mode=END_TO_START;
   }
  break;
  
  case END_TO_START:              //从末尾回到开头
   {
    time_end_to_start++;
    set_speed(-100);
   }
  break;
  
  case OVER:                      //全程结束
   MMTR_STOP();   
   Set_Angle(0);
   for(unsigned int kk=0;kk<50000;kk++);
   TA_STOP();
  break;
  
  case NEXT_BAL:
   time_next_bal++;
   if(time_next_bal>30)           //三秒钟内将重物放好
   {
    time_bal_search2=time_bal_search; //保存第一次平衡时查找时间
    time_bal_wait2=50;
    time_bal_search=0;
    work_mode=BAL_SEARCH;
    sys_mode=1;
   };
   break;
   
  default:break;
 }
}
