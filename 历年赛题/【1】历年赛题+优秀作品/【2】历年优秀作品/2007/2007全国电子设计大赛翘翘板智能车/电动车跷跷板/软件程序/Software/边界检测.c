#include<msp430x14x.h>
#include"边界检测.h"

#define FL BIT4           //P5
#define FM BIT3
#define FR BIT2
#define SEL_F BIT1        //P5
#define BL BIT5           //P1
#define BM BIT6
#define BR BIT7           //P1
#define SEL_B BIT1        //P3

#define SEL_F_ON  P5OUT|=SEL_F          //前部传感器选通开
#define SEL_F_OFF P5OUT&=~SEL_F
#define SEL_B_ON  P3OUT|=SEL_B          //后部传感器选通关
#define SEL_B_OFF P3OUT&=~SEL_B



signed char fr_flag,fl_flag,br_flag,bl_flag;
unsigned char ftemp,btemp; 
signed char fl_state[5],fr_state[5],bl_state[5],br_state[5];    //存放四角近5次数据

#define  FREAD(BIT)  (ftemp&BIT)
#define  BREAD(BIT)  (btemp&BIT)

void sensor_init()                //管教初始化
{
 P5DIR|=SEL_F;
 P5DIR&=~(FL+FM+FR);
 P3DIR|=SEL_B;
 P1DIR&=~(BL+BM+BR);
 P5OUT&=~SEL_F;
 P3OUT&=~SEL_B;
}

void sel_on()                     //传感器选通开
{
 SEL_F_ON;
 SEL_B_ON;
}

void sel_off()                    //传感器选通开
{
 SEL_F_OFF;
 SEL_B_OFF;
}

unsigned char First_Flag; 

void sensor_scan()
{
  unsigned int i;
  
  sel_on();                       //开传感器延时0.4us关闭
  for(i=500;i>0;i--);
  ftemp=P5IN&(FL+FR);
  btemp=P1IN&(BL+BR); 
  sel_off();
  
  if(FREAD(FL)==0)fl_flag=-1;     //读取各传感器值并存入
    else fl_flag=0;
  if(FREAD(FR)==0)fr_flag=1;
    else fr_flag=0;
  if(BREAD(BL)==0)bl_flag=-1;
    else bl_flag=0;
  if(BREAD(BR)==0)br_flag=1;
    else br_flag=0; 
    
  if(First_Flag==0)             //初始化各数据
  {
    First_Flag=1;
    for(i=0;i<5;i++)
    {
     fl_state[i]=-1;
     fr_state[i]=1;
     bl_state[i]=1;
     br_state[i]=-1;
    }
  }
   for(i=4;i>0;i--)             //储存5次内边界数据
   {
    fl_state[i]=fl_state[i-1];
    fr_state[i]=fr_state[i-1];
    bl_state[i]=bl_state[i-1];
    br_state[i]=br_state[i-1];
   }
  fl_state[0]=fl_flag;
  fr_state[0]=fr_flag;
  bl_state[0]=bl_flag;
  br_state[0]=br_flag;
  
}

