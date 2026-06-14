#include<msp430x14x.h>
#include"key.h"

#define KEY_ON 0
#define KEY_OFF 1

#define KEY BIT3
#define READ_KEY (P3IN&KEY)
#define long_key 8

unsigned char now_key;
unsigned char pre_key;
unsigned char key_count;
unsigned char short_flag;
unsigned char long_flag;
/*-------------------------------------------
  函数名：key_init()
  函数功能：按键初始化程序
--------------------------------------------*/
void key_init()                       
{
 pre_key=KEY_OFF;
 now_key=KEY_OFF;
 P3DIR&=~KEY;
 P3OUT|=KEY;
}
/*-------------------------------------------
  函数名：key_scan()
  函数功能：按键扫描
  说明：如果检测到短按，则short_flag=1
        如果检测到长按，则long_flag=1
--------------------------------------------*/
void key_scan()
{
 pre_key=now_key;
 now_key=READ_KEY;
 if((pre_key==KEY_ON)&&(now_key!=KEY_ON))           //按键松开认为是短按
   short_flag=1;
 else short_flag=0;
 if((pre_key==KEY_ON)&&(now_key==KEY_ON))           //长按时间超过long_key认为是长按
 {
   key_count++;
   if(key_count>long_key)
   {
     key_count=long_key;
     long_flag=1;
   }
 }
 else 
   {
     key_count=0;
     long_flag=0;
   }
}
