//-----------------------------------------------------------------
// 程序描述:
// 		 IIC驱动头文件
// 作    者: 凌智电子
// 开始日期: 2020-11-11
// 完成日期: 2020-11-11
// 修改日期: 
// 当前版本: V1.0
// 历史版本:
//  - V1.0: (2020-11-11)IIC驱动头文件
// 调试工具: 凌智STM32H750核心板、LZE_ST_LINK2
// 说    明: 
//    
//-----------------------------------------------------------------
#ifndef __IIC_H
#define __IIC_H
#include "lz_hardware.h"
#define IIC_SCL   GPIO0_PA1
#define IIC_SDA   GPIO0_PA0


//-----------------------------------------------------------------
// 宏定义
//-----------------------------------------------------------------
//IO方向设置
#define IIC_SDA_IN()   LzGpioSetDir(IIC_SDA, LZGPIO_DIR_IN);
#define IIC_SDA_OUT() LzGpioSetDir(IIC_SDA, LZGPIO_DIR_OUT);

#define IIC_SCL_L 		  LzGpioSetVal(IIC_SCL, LZGPIO_LEVEL_LOW)							
#define IIC_SCL_H 		  LzGpioSetVal(IIC_SCL, LZGPIO_LEVEL_HIGH)			

#define IIC_SDA_L 		 LzGpioSetVal(IIC_SDA, LZGPIO_LEVEL_LOW)						
#define IIC_SDA_H        LzGpioSetVal(IIC_SDA, LZGPIO_LEVEL_HIGH)


//-----------------------------------------------------------------
// 外部函数声明
//-----------------------------------------------------------------
extern void IIC_Delay(void);	// IIC延时函数
extern void IIC_Init(void);		// IIC初始化
extern void IIC_Start(void);	// 产生IIC起始信号
extern void IIC_Stop(void);		// 产生IIC停止信号
extern unsigned char IIC_Wait_Ack(void);	// 等待应答信号
extern void IIC_Ack(void);		// 产生ACK应答
extern void IIC_NAck(void);		// 不产生ACK应答
extern void IIC_Send_Byte(uint8_t txd);	// IIC发送一个字节
extern unsigned char IIC_Read_Byte(uint8_t ack);	// IIC读取一个字节
uint8_t temp_Read_Len(uint8_t cmd, uint8_t *Buf, uint8_t Len);
#endif
//-----------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------













