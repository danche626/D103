#include <MSP430X14X.h>
#include"舵机.H"
#include"边界检测.h"
#include"主电机.h"
#include"global.h"

void Backing();

signed char angle=0;
signed char pre_angle=0;

void Edge_Control()            //转弯控制算法
{
 
  unsigned char i;

  if(dir_flag==1)               //正走模式
  {
    for(i=0;i<2;i++)            //前面连续两次没有探测到板面，认定为板末尾
    {
     if((fl_state[i]==0)&&(fr_state[i]==0))  
     continue;
     else break;
    }
    if(i==2)
    {
      MMTR_STOP();              //刹车并进入末尾等待模式
      work_mode=END_WAIT;
    }
    else                        //检测到板子出界，调整舵机角度，继续前进
    {
     angle=0;
     for(i=0;i<5;i++)
     {
      angle=angle+(fl_state[i]+fr_state[i]);    //舵机转动范围-5~+5
     }    
     if(angle>5)angle=5;
     if(angle<-5)angle=-5;
     Set_Angle(angle); 
    }
  }
  else                          //倒车模式，结构同正走模式
  {  
    for(i=0;i<2;i++)
    {
     if((bl_state[i]==0)&&(br_state[i]==0))
     continue;
     else break;
    }
    if(i==2)
    {
      MMTR_STOP();              //检测回到起始点，整个过程结束
      work_mode=OVER;
    } 
    else
    Backing();                  //倒车处理程序
  }    
}

unsigned char Backing_Count=0;
unsigned char Backing_Time=0;
unsigned char Backing_Temp=0;
unsigned char Backing_Mode=0;
unsigned char start_flag;

void Backing()
{  
  switch(Backing_Mode)
  {    
  case 0:                                         //检测车是否有偏转
  {
    Set_Angle(0); 
    if((bl_state[0]==0)&&(bl_state[1]==0))        //检测到车靠左了
      Backing_Mode=1;
    else if((br_state[0]==0)&&(br_state[1]==0))   //检测到车靠右了
      Backing_Mode=11;
  }
  break;
  
  case 1:                           //车偏左，右转角度4，对左后轮出界时间进行计时，直到左前轮也出界
  {  
    if(bl_flag==0)
    {
      start_flag=1;
    };  
    if(fl_flag==0)                  //左前轮出界，停止计时，进入下一模式
    {
     start_flag=0;
     Backing_Time=Backing_Count;    //最终计时存入Backing_Time
     Backing_Count=0;
     Backing_Mode=2;
    };
    if(start_flag==1)
    {
      Backing_Count++;
    }
    Set_Angle(4);
  }
  break;
  
  case 2:                           //右转角度3，直到左后轮回归板上，
  {
    if(bl_flag==0)
    {
      Set_Angle(3);
    }  
    else
    {
      Backing_Temp=0;
      Backing_Mode=3;
    }  
  }
  break;
   
  case 3:                           //以Backing_Time*0.8，短时间左转角度5，校正小车回直线
  {
    if(++Backing_Temp < Backing_Time*0.7)   //0.7
    {
      Set_Angle(-5);
    }  
    else
    {
      Backing_Temp=0;
      Backing_Mode=0;
    }    
  }  
  break; 
  
  case 11:                            //车偏右，左转角度4，对右后轮出界时间进行计时，直到右前轮也出界
  {  
    if(br_flag==0)
    {
      start_flag=1;
    };  
    if(fr_flag==0)
    {
     start_flag=0;
     Backing_Time=Backing_Count;
     Backing_Count=0;
     Backing_Mode=12;
    };
    if(start_flag==1)
    {
      Backing_Count++;
    }
    Set_Angle(-4);
  }
  break;
  
  case 12:                           //左转角度3，直到右后轮回归板上，
  {
    if(br_flag==0)
    {
      Set_Angle(-3);
    }  
    else
    {
      Backing_Temp=0;
      Backing_Mode=13;
    }  
  }
  break;
   
  case 13:                           //以Backing_Time*0.8，短时间右转角度5，校正小车回直线
  {
    if(++Backing_Temp < Backing_Time*0.7)
    {
      Set_Angle(5);
    }  
    else
    {
      Backing_Temp=0;
      Backing_Mode=0;
    }    
  }  
  break; 
  
  }//switch end   

}
