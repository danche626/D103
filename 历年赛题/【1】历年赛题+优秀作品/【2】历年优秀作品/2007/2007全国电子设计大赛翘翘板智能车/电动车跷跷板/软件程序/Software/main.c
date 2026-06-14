#include<msp430x14x.h>
#include"边界检测.H"
#include"LCD1602.H"
#include"舵机.h"
#include"主电机.h"
#include"Edge_Control.h"
#include"key.h"
#include"倾角.h"
#include"global.h"
#include"time.h"
#include"display.h"

/*-------------------------------------------
  函数名：cpu_sleep_delay()
  函数功能：系统进入低功耗工作模式 LPM1
--------------------------------------------*/
unsigned char sleep_flag;
void cpu_sleep_delay()
{
 while(sleep_flag)LPM1;
 sleep_flag=1;
}
/*-------------------------------------------
  函数名：XT2_Init()
  函数功能：外部高速晶振设定
--------------------------------------------*/

void XT2_Init(void)
{
  unsigned char i;
  BCSCTL1 &= ~(XT2OFF);  
  do{
    IFG1 &= ~(OFIFG);                                                                                                                                                                                                                                                                                                                      
    for(i=0; i<100; i++){_NOP();}
  }while((IFG1 & OFIFG) != 0);
}

/*-------------------------------------------
  函数名：BCS_init()
  函数功能：系统时钟设定

  说明： DCO设置为 1000KHz
         ACLK 为 LFXT1(低频模式)
         SMLCK为 DCOCLK 
         MCLK 为 4MHz
--------------------------------------------*/
void BCS_init(void)
{
  XT2_Init();
  DCOCTL  = 0xC0 + 0x00;                   
  BCSCTL1 = DIVA_0 + 0x04;
  BCSCTL2 = SELM_2 + DIVM_0 + DIVS_0;      
}
/*-------------------------------------------
  函数名：WDT_init()
  函数功能：看门狗设置

  说明：配置看门狗为定时中断模式----8MS定时
        开启中断 
--------------------------------------------*/
void WDT_init()
{
  WDTCTL=WDT_MDLY_32;     //32MS定时
  IE1|=WDTIE;
  _EINT();
}
/*--------------------------------------------
  函数名：sys_init()
  函数功能：系统参数初始化
---------------------------------------------*/  
void sys_init()
{  
  BCS_init();           //系统时钟
  WDT_init();           //看门狗定时器
  sensor_init();        //边界检测红外传感器
  key_init();           //按键
  LCD_init();           //LCD
  MMTR_INIT();          //主电机
  PWM_SM_INIT();        //舵机
  SPI_Init();           //SCA100T倾角传感器
  time_init();          //各计时变量初始化
  Set_Angle(0);         //将舵机初始角度设为零
  TA_START();           //开启舵机
  MMTR_START();         //开启主电机
}
/*-------------------------------------------
  函数名：yyfilter()
  函数功能：角度取值滤波
--------------------------------------------*/
/*unsigned char yy_cnt=0;
signed int yy_temp;
unsigned char yyfilter(unsigned char samplenum)
{       
  if(yy_cnt>=samplenum)
  {
    yy_cnt=0;
    yy_temp=0;
  }
  yy_cnt++;
  yy=SPI_GetY();      
  yy_temp+=yy;
  if(yy_cnt>=samplenum)                                  //取samplenum次平均值
  {    
   yy_temp/=samplenum;
   yy_avrg=yy_temp;
  }
  return yy_cnt;
}*/
 
/*-------------------------------------------
  函数名：speed_control()
  函数功能：速度控制
--------------------------------------------*/
/*unsigned int speed_cnt;
signed char speed_dir_flag;
signed char speed_wait_cnt;
unsigned char speed_zero_cnt;
signed char speed_wait;
#define SPEED_ZERO 0       //等待稳定时间 (SPEED_ZERO+1) * 32ms
#define YY_SAMPLE 10       //yy采样次数

void speed_control()
{ 
  if((work_mode==BAL_SEARCH)||(work_mode==BAL_WAIT)) //在平衡查找模式及等待模式进行速度控制
  {   
    speed_wait_cnt++;
    if(speed_wait_cnt>speed_wait)         //某一速度持续时间
    {
     speed_wait_cnt=speed_wait;
     MMTR_STOP();
     speed_zero_cnt++;                      //等待稳定
     if(speed_zero_cnt>SPEED_ZERO)
     {
      speed_zero_cnt=SPEED_ZERO;
      if(yyfilter(YY_SAMPLE)>=YY_SAMPLE)
      {
        speed_wait_cnt=0;
        speed_wait=1;
        yy_cnt=0;
        if((yy_avrg>30)||(yy_avrg<-30))
          balance_flag=0;          
        if(yy_avrg>30)
          speed_dir_flag=-1;
        else if(yy_avrg<-30)
          speed_dir_flag=1;
        if( (yy_avrg>=-30)&&(yy_avrg<=30) )           //±30以内 停下
         {
          speed_wait=0;
          balance_flag=1;
         }
        else if( (yy_avrg>=-100)&&(yy_avrg<=100) )      //±100以内 慢行
           set_speed(20*speed_dir_flag);
        else if( (yy_avrg>=-160)&&(yy_avrg<=160) )      //±100~±160 爬坡
           set_speed(30*speed_dir_flag);
        else
           set_speed(40*speed_dir_flag);                         //>±160全速 
      }
     }
   }   
  }
}*/
unsigned char yy_cnt=0;
signed int yy_temp;
signed char speed_dir_flag;
signed char speed_wait_cnt;
unsigned char speed_zero_cnt;
signed char speed_wait;
#define SPEED_ZERO 0       //等待稳定时间 (SPEED_ZERO+1) * 32ms
#define YY_SAMPLE 10       //yy采样次数
#define BAL_ANGLE 40       //平衡时角度
void speed_control()
{ 
  if((work_mode==BAL_SEARCH)||(work_mode==BAL_WAIT))  //在平衡查找模式及等待模式进行速度控制
  {   
    speed_wait_cnt++;
    if(speed_wait_cnt>speed_wait)                     //某一速度持续时间
    {
     speed_wait_cnt=speed_wait;
     MMTR_STOP();
     speed_zero_cnt++;                                //等待稳定
     if(speed_zero_cnt>SPEED_ZERO)
     {
      speed_zero_cnt=SPEED_ZERO;
      yy_cnt++;
      yy=SPI_GetY();      
      yy_temp+=yy;
      if(yy_cnt>=YY_SAMPLE)                           //取十次平均值
      {           
       yy_temp/=YY_SAMPLE;
       yy_avrg=yy_temp;
       speed_wait_cnt=0;
       speed_wait=1;
       yy_cnt=0;      
       if((yy_avrg>BAL_ANGLE)||(yy_avrg<-BAL_ANGLE))
       {
         balance_flag=0;
         time_bal_insure=0;
       }
       else balance_flag=1;
        if(yy_avrg>BAL_ANGLE)
          speed_dir_flag=-1;
        else if(yy_avrg<-BAL_ANGLE)
          speed_dir_flag=1;
        if( (yy_avrg>=-BAL_ANGLE)&&(yy_avrg<=BAL_ANGLE) ) //±40以内 停下
         {
           speed_wait=0;
         }
        else if( (yy_avrg>=-100)&&(yy_avrg<=100) )      //±160以内 慢行
          set_speed(20*speed_dir_flag);
        else if( (yy_avrg>=-160)&&(yy_avrg<=160) )      //±120~±160 爬坡
          set_speed(30*speed_dir_flag);
        else
          set_speed(40*speed_dir_flag);                 //>±全速   
         yy_temp=0;
      }
     }
   }   
  }
}

/*-------------------------------------------
  函数名：ShangBan()
  函数功能：上板过程
 -------------------------------------------*/
unsigned char Shangban_Mode=1;
signed int xx_std=0,xx_now=0,yy_std=0;

void ShangBan()
{ 
  unsigned char i=0;  
  while(1)                              //系统模式二，上板过程
  {
    cpu_sleep_delay(); 
    switch(Shangban_Mode)
    {
      case 1:                             //停车预取样xx_std和yy_std
      {        
        xx_std+=SPI_GetX();       
        yy_std+=SPI_GetY();       
        if(++i>=15)                       //15次预取样完成
        {
          xx_std/=i;
          yy_std/=i;
          i=0;
          LCD_clear();
          gotoXY(0,5);
          print_num(xx_std,3);
          gotoXY(1,5);
          print_num(yy_std,3);
          set_speed(100);
          for(unsigned int mm=0;mm<50000;mm++) //防止车启动瞬间的误触发
           for(unsigned int nn=0;nn<2;nn++);
          Shangban_Mode=2;
        }        
      }
      break;
    
      case 2:                                  //探测前轮是否上板
      {              
        yy=SPI_GetY();
        if((yy-yy_std)<-190)                            //前轮已上板
        {
          LCD_clear();
          gotoXY(0,5);
          print_num(yy-yy_std,3);
          Shangban_Mode=3;
          MMTR_STOP();
          for(unsigned int mm=0;mm<10000;mm++) //等待车停稳
           for(unsigned int nn=0;nn<3;nn++);
        }      
      }
      break;    
    
      case 3:                             //停车取样xx_now
      {
        xx_now+=SPI_GetX();
        if(++i>=15)                       //停车后15次xx取样完成
        {
          xx_now/=i;
          i=0;
          Shangban_Mode=4;
        }        
      }
      break;
      
      case 4:                             //由xx_now-xx_std决定前轮的转弯量
      {            
          signed int xx_temp;          
          xx_temp=xx_now-xx_std; 
          LCD_clear();
          gotoXY(0,5);
          print_num(xx_temp,3);
          gotoXY(1,5);
          print("Turning");
          if(  (xx_temp<=10)&&(xx_temp>=-10)  )       //-10~10
            Set_Angle(0);
          else if(  (xx_temp<=20)&&(xx_temp>=10)  )   //10~20
            Set_Angle(3);
          else if(  (xx_temp>=20) )                   //>20
            Set_Angle(5);
          else if(  (xx_temp<=-10)&&(xx_temp>=-20)  ) //-10~-20
            Set_Angle(-3);
          else                                        //<-20
            Set_Angle(-5);
          set_speed(100);
          
          sensor_scan();                    //提前开始扫描，避免完全上板后错误转弯
        if(++i>=60)                         //延时2秒等待小车基本上板
        {
          i=0;
          return;
        }      
      }
      break;          
    }//end switch
  }
}
/*-------------------------------------------
  函数名：sys_mode_set()
  函数功能：人机交互，进行系统模式设定
  说明：短按按键切换模式
        长按确定当前模式并退出
--------------------------------------------*/
unsigned char sys_mode_set()
{
  unsigned char temp=0;
  while(1)                      //人机交互等待命令
  {
  cpu_sleep_delay();
  glb_cnt++;  
  if(glb_cnt>=3)   
  {
   glb_cnt=0;
   key_scan();
   LCD_clear();   
   if(long_flag==1)             //长按后确认退出While(1)
   {
     return (temp+1);
   }
   else if(short_flag==1)       //短按切换系统模式
    temp=(temp+1)%2;
   if(temp==0)
   {
    gotoXY(1,4);   
    print("mode:");
    print_time(temp+1);
   }   
   else
   {
    gotoXY(1,4);
    print("mode:");
    print_time(temp+1);
   }   
   gotoXY(0,4);
   print("Welcome!");
  }
  }//end while
}


/*-------------------------------------------
  函数名：main()
  函数功能：主函数
--------------------------------------------*/
void  main( void )
{  
  dir_flag=1;                   //先设置成正走模式
  sys_init();                   //系统初始化
  sys_mode=sys_mode_set();      //系统工作模式设定
  
  LCD_clear();                    //显示模式设置完成，请求释放
  gotoXY(0,1);
  print("Mode Set Over!");
  gotoXY(1,1);
  print("Please Release!");
  
  for(unsigned int mm=0;mm<50000;mm++)
    for(unsigned int nn=0;nn<30;nn++);
  
  if(sys_mode==2)                 //如果系统模式为2，则进入上板程序
    ShangBan();   
  
  while(1)
  {
     cpu_sleep_delay();            //430进入休眠，每32ms唤醒一次
     glb_cnt++;  
     speed_control();              //速度调整       
     if(glb_cnt>=3)                //每0.1S执行一次
      {
       time_count();
       display();
       if((work_mode!=BAL_SEARCH)&&(work_mode!=BAL_WAIT)&&(work_mode!=END_WAIT))
       {             
        sensor_scan();              //边界检测
        Edge_Control();             //转弯控制
       }
       glb_cnt=0;
      }
  }
}
/*-------------------------------------------
  函数名： WDT_ISR()
  函数功能： 看门狗中断服务
--------------------------------------------*/
#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR()
{
 sleep_flag=0;   // 退出低功耗模式
 LPM1_EXIT;
}
