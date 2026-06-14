#ifndef __MYIIC_HW_H
#define __MYIIC_HW_H

#include "./SYSTEM/sys/sys.h"  // 复用原有系统头文件
#include "FreeRTOS.h"
#include "task.h"

/************************ 硬件IIC配置 ************************/
#define IIC_HW_I2Cx                I2C2                // 使用I2C2外设
#define IIC_HW_CLK_ENABLE()        __HAL_RCC_I2C2_CLK_ENABLE()  // I2C2时钟使能
#define IIC_HW_GPIO_PORT           GPIOB
#define IIC_HW_SCL_PIN             GPIO_PIN_10
#define IIC_HW_SDA_PIN             GPIO_PIN_11
#define IIC_HW_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()  // GPIOB时钟使能
#define IIC_HW_TIMEOUT             1000                // IIC超时时间(ms)

/* I2C句柄声明 */
extern I2C_HandleTypeDef hi2c2;

/************************ 兼容原模拟IIC的函数接口 ************************/
void iic_init(void);                                    // 替换原模拟IIC初始化
void iic_write_register(uint8_t i2c_addr, uint8_t reg_addr, uint8_t data); // 兼容原函数名
uint8_t iic_read_register(uint8_t i2c_addr, uint8_t reg_addr);             // 兼容原函数名
uint8_t iic_read_len(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len); // 兼容原函数名

#endif

/*
// 测试代码（与原模拟IIC调用方式完全一致）
int main(void)
{
    HAL_Init();
    sys_stm32_clock_init(336, 8, 2, 7); // 初始化时钟（根据MCU修改）
    delay_init(168);
    iic_init();  // 硬件IIC初始化（替换原模拟IIC）
    
    // 写入测试：向0x48设备的0x10寄存器写0x55
    iic_write_register(0x48, 0x10, 0x55);
    
    // 读取测试：从0x48设备的0x10寄存器读数据
    uint8_t data = iic_read_register(0x48, 0x10);
    
    // 多字节读取测试
    uint8_t buf[5];
    iic_read_len(0x48, 0x20, buf, 5);
    
    while(1);
}
*/

