#include<msp430x14x.h>

/*************************************
  RS=P6^2;
  RW=P6^1;
  EN=P6^0;
  LIGHT=P5^7
*************************************/

#define RS BIT2
#define RW BIT1
#define EN BIT0
#define LIGHT BIT7
#define DataPort P2OUT

#define SETB(BIT) P6OUT|=BIT
#define CLRB(BIT) P6OUT&=~BIT

unsigned char count;

unsigned char IsBusy()
{
 unsigned char busy;
 CLRB(RS);	   
 SETB(RW);	   //_nop_();
 SETB(EN);	   //_nop_();
 //while(P0&0x80);
 P2DIR&=~BIT7;
 busy=(P2IN&BIT7);
 P2DIR|=BIT7;
 CLRB(EN);	  // _nop_();
 return busy;
}	 
//向LCD写入命令或数据************************************************************
#define LCD_COMMAND			0      // Command
#define LCD_DATA			1      // Data
#define LCD_CLEAR_SCREEN	0x01      // 清屏
#define LCD_HOMING  		0x02      // 光标返回原点

void LCD_Write(unsigned char style,unsigned char input)
{
 while(IsBusy());
 CLRB(EN);
 if(style)SETB(RS);
 else CLRB(RS);
 CLRB(RW);   //_nop_();
 DataPort=input; //_nop_();
 SETB(EN);     //_nop_();
 CLRB(EN);	   //_nop_();
}
//设置显示模式
#define LCD_SHOW			0x04    //显示开
#define LCD_HIDE			0x00    //显示关	    

#define LCD_CURSOR			0x02 	//显示光标
#define LCD_NO_CURSOR		0x00    //无光标		     

#define LCD_FLASH			0x01    //光标闪动
#define LCD_NO_FLASH		0x00    //光标不闪动

void LCD_SetDisMode(unsigned char DisMode)
{
 LCD_Write(LCD_COMMAND,0x08|DisMode);
}
//设置画面模式
#define LCD_AC_UP			0x02
#define LCD_AC_DOWN			0x00      // default

#define LCD_MOVE			0x01      // 画面可平移
#define LCD_NO_MOVE			0x00      //default

void LCD_SetInputMode(unsigned char InputMode)
{
 LCD_Write(LCD_COMMAND,0x40|InputMode);
}
void LCD_ClrScr()
{
 LCD_Write(LCD_COMMAND,LCD_CLEAR_SCREEN);
}
//初始化
void LCD_init()
{
 P2DIR=0XFF;
 DataPort=0x00;                                   
 P6DIR|=(RS+RW+EN);
 P6OUT&=~(RS+RW+EN);
 P5DIR|=LIGHT;
 P5OUT&=~LIGHT;
 
 LCD_Write(LCD_COMMAND,0x38);
 LCD_Write(LCD_COMMAND,0x38);
 LCD_SetDisMode(LCD_SHOW+LCD_NO_CURSOR);
 LCD_ClrScr();
 LCD_SetInputMode(LCD_AC_UP+LCD_NO_MOVE);
}
void gotoXY(unsigned char x, unsigned char y)
{
  LCD_Write(LCD_COMMAND,0x80|(y+0x40*x));
}
//显示字符串
void print(unsigned char *str)
{
	while(*str!='\0')
	{
		LCD_Write(LCD_DATA,*str);
		str++;
	}
}
void print_num(signed int num,unsigned char bitnum)
{
 unsigned char num_temp[5];
 unsigned char i;

 if(num<0)
 {
  num=-num;
  num_temp[4]='-';
 }
 else
  num_temp[4]='+'; 
 
 for(i=(4-bitnum);i<4;i++)
 {
  num_temp[i]=num%10;  
  num/=10;
 }
 LCD_Write(LCD_DATA,num_temp[4]);
 for(i=4;i>(4-bitnum);i--)
 {
  LCD_Write(LCD_DATA,num_temp[i-1]+48);
 }
}
void print_time(unsigned int num)
{
 unsigned char num_temp[3];
 unsigned char i;

 for(i=0;i<3;i++)
 {
  num_temp[i]=num%10;  
  num/=10;
 }
 for(i=3;i>0;i--)
 {
  LCD_Write(LCD_DATA,num_temp[i-1]+48);
 }
}
void light_on()
{
 P5OUT|=LIGHT;
}
void light_off()
{
 P5OUT&=~LIGHT;
}
void LCD_clear()
{
 LCD_Write(LCD_COMMAND,LCD_CLEAR_SCREEN);
}
/*void auto_off()
{
 if((short_flag==0)&&(long_flag==0))
 {
  count++;
  if(count>long)
  {
   count=long;
   light_off();
  }
 }
}*/
