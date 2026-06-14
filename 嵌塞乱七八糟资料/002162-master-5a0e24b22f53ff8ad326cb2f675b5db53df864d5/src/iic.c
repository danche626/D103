//-----------------------------------------------------------------
// 程序描述:
//     IIC驱动程序
// 作    者: 凌智电子
// 开始日期: 2020-11-11
// 完成日期: 2020-11-11
// 修改日期: 
// 当前版本: V1.0
// 历史版本:
//  - V1.0: (2020-11-11)IIC驱动
// 调试工具: 凌智STM32H750核心板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------
#define TM_I2C_ADDRESS      0x00

//-----------------------------------------------------------------
// 头文件包含
//-----------------------------------------------------------------

#include "iic.h"
#include "lz_hardware.h"
 //-----------------------------------------------------------------
 unsigned char IIC_SDA_READ()
 {
     unsigned char val;
     LzGpioGetVal(IIC_SDA,&val);
     return val;
 }
 
//-----------------------------------------------------------------
// void IIC_Delay(void)
//-----------------------------------------------------------------
// 
// 函数功能: IIC 延时函数
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void IIC_Delay(void)
{
	ToyUdelay(2);
}

//-----------------------------------------------------------------
// void IIC_Init(void)
//-----------------------------------------------------------------
// 
// 函数功能: IIC初始化
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void IIC_Init(void)
{					  
    PinctrlSet(IIC_SCL, MUX_FUNC0, PULL_KEEP, DRIVE_KEEP);
    LzGpioInit(IIC_SCL);
    LzGpioSetDir(IIC_SCL, LZGPIO_DIR_OUT);
    LzGpioSetVal(IIC_SCL, LZGPIO_LEVEL_HIGH);	
	

    PinctrlSet(IIC_SDA, MUX_FUNC0, PULL_KEEP, DRIVE_KEEP);  
    LzGpioInit(IIC_SDA);
    LzGpioSetDir(IIC_SDA, LZGPIO_DIR_OUT);
    LzGpioSetVal(IIC_SDA, LZGPIO_LEVEL_HIGH);

}


//-----------------------------------------------------------------
// void IIC_Start(void)
//-----------------------------------------------------------------
// 
// 函数功能: 产生IIC起始信号
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void IIC_Start(void)
{
	// 当SCL高电平时，SDA出现一个下跳沿表示I2C总线启动信号
	IIC_SDA_OUT();
	IIC_SDA_H;	  	  
	IIC_SCL_H;
	IIC_Delay();
 	IIC_SDA_L;
	ToyUdelay(40);
	IIC_Delay();
	IIC_SCL_L;
}	  

//-----------------------------------------------------------------
// void IIC_Stop(void)
//-----------------------------------------------------------------
// 
// 函数功能: 产生IIC停止信号
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void IIC_Stop(void)
{
	// 当SCL高电平时，SDA出现一个上跳沿表示I2C总线停止信号
	IIC_SDA_OUT();
	IIC_SCL_L;
	IIC_SDA_L;
 	IIC_Delay();
	IIC_SCL_H;  
	IIC_SDA_H;
	IIC_Delay();							   	
}

//-----------------------------------------------------------------
// uint8_t IIC_Wait_Ack(void)
//-----------------------------------------------------------------
// 
// 函数功能: 等待应答信号
// 入口参数: 无 
// 返 回 值: 1:接收应答失败，0:接收应答成功
// 注意事项: 无
//
//-----------------------------------------------------------------
uint8_t IIC_Wait_Ack(void)
{
	uint8_t OutTime=0;
	IIC_SDA_IN();
	IIC_SDA_H;
	IIC_Delay();	   
	IIC_SCL_H;
	IIC_Delay();	 
	while(IIC_SDA_READ())
	{
		OutTime++;
		if(OutTime>250)
		{
			IIC_Stop();
			return 1;
		}
	}
	IIC_SCL_L;	   
	return 0;  
} 

//-----------------------------------------------------------------
// void IIC_Ack(void)
//-----------------------------------------------------------------
// 
// 函数功能: 产生ACK应答
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void IIC_Ack(void)
{
	IIC_SCL_L;
	IIC_SDA_OUT();
	IIC_SDA_L;
	IIC_Delay();
	IIC_SCL_H;
	IIC_Delay();
	IIC_SCL_L;
}

//-----------------------------------------------------------------
// void IIC_NAck(void)
//-----------------------------------------------------------------
// 
// 函数功能: 不产生ACK应答		
// 入口参数: 无 
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void IIC_NAck(void)
{
	IIC_SCL_L;
	IIC_SDA_OUT();
	IIC_SDA_H;
	IIC_Delay();
	IIC_SCL_H;
	IIC_Delay();
	IIC_SCL_L;
}					 				     

//-----------------------------------------------------------------
// void IIC_Send_Byte(uint8_t txd)
//-----------------------------------------------------------------
// 
// 函数功能: IIC发送一个字节	
// 入口参数: unsigned char txd：要发送的字节
// 返 回 值: 无
// 注意事项: 无
//
//-----------------------------------------------------------------
void IIC_Send_Byte(uint8_t txd)
{                        
	uint8_t t;   
	IIC_SDA_OUT(); 	    
	IIC_SCL_L;
	for(t=0;t<8;t++)
	{   
		if((txd&0x80)>>7)
			IIC_SDA_H;
		else IIC_SDA_L;
		txd<<=1; 	  
		IIC_SCL_H;
		IIC_Delay(); 
		IIC_SCL_L;	
		IIC_Delay();
	}	 
} 	    

//-----------------------------------------------------------------
// uint8_t IIC_Read_Byte(uint8_t ack)
//-----------------------------------------------------------------
// 
// 函数功能: IIC读取一个字节	
// 入口参数: uint8_t ack：ack=1时，发送ACK，ack=0，发送nACK
// 返 回 值: 读取的字节
// 注意事项: 无
//
//-----------------------------------------------------------------
uint8_t IIC_Read_Byte(uint8_t ack)
{
	uint8_t i,receive=0;
	IIC_SDA_IN();
  for(i=0;i<8;i++ )
	{
		IIC_SCL_L; 
		IIC_Delay();
		IIC_SCL_H;
		receive<<=1;
		if(IIC_SDA_READ())
			receive++;   
		IIC_Delay(); 
   }					 
	if (!ack)
			IIC_NAck();//发送nACK
	else
			IIC_Ack(); //发送ACK   
	return receive;
}

uint8_t temp_Read_Len(uint8_t cmd, uint8_t *Buf, uint8_t Len)
{ 
 	IIC_Start(); 
	IIC_Send_Byte(TM_I2C_ADDRESS);	// 发送器件地址+写命令	
	if(IIC_Wait_Ack())					// 等待应答
	{
		IIC_Stop();		 
		return 1;		
	}
	IIC_Send_Byte(cmd);					// 写寄存器地址
	IIC_Wait_Ack();							// 等待应答
	IIC_Start();
	IIC_Send_Byte(TM_I2C_ADDRESS|0x01);	//  发送器件地址+读命令	
	IIC_Wait_Ack();							//等待应答 
	while(Len)
	{
		if(Len==1)
			*Buf=IIC_Read_Byte(0);	// 读数据,发送nACK 
		else 
			*Buf=IIC_Read_Byte(1);				// 读数据,发送ACK  
		Len--;
		Buf++; 
	}    
  IIC_Stop();	// 产生一个停止条件 
	return 0;	
}

//-----------------------------------------------------------------
// End Of File
//----------------------------------------------------------------- 
