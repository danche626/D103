#include "iic.h"

void i2c_init(void)
{
    GPIO_InitTypeDef gpio_initstruct;

    I2C_SCL_CLK();
    I2C_SDA_CLK();
    
    gpio_initstruct.Pin = I2C_SCL2_PIN;          
    gpio_initstruct.Mode = GPIO_MODE_OUTPUT_OD;             
    gpio_initstruct.Pull = GPIO_PULLUP;                     
    gpio_initstruct.Speed = GPIO_SPEED_FREQ_HIGH;           
    HAL_GPIO_Init(I2C_SCL_PORT, &gpio_initstruct);
    
    gpio_initstruct.Pin = I2C_SDA2_PIN;          
    HAL_GPIO_Init(I2C_SDA_PORT, &gpio_initstruct);
}
void i2c_start(void)
{
    I2C_SDA_SET();
    I2C_SCL_SET();
    I2C_SDA_RESET();
    I2C_SCL_RESET();
}

void i2c_stop(void)
{
    I2C_SDA_RESET();
    I2C_SCL_SET();
    I2C_SDA_SET();
}

void i2c_send_ack(uint8_t ack)
{
    if(ack == 1)
        I2C_SDA_SET();// NACK
    else
        I2C_SDA_RESET();// ACK
    
    I2C_SCL_SET();
    I2C_SCL_RESET();
}

uint8_t i2c_receive_ack(void)
{
    uint8_t ack;
    I2C_SDA_SET();
    I2C_SCL_SET();
		SDA_IN();
    ack = I2C_SDA_VALUE();
		SDA_OUT();
    I2C_SCL_RESET();
    return ack;
}

void i2c_send_byte(uint8_t data)
{
    uint8_t i, tmp;
    tmp = data;
    
    for(i = 0; i < 8; i++)
    {
        if((tmp & 0x80) == 0x80)
            I2C_SDA_SET();
        else
            I2C_SDA_RESET();
        tmp = tmp << 1;
        I2C_SCL_SET();
        I2C_SCL_RESET();
    }
}

uint8_t i2c_receive_byte(void)
{
    uint8_t i = 0, data = 0;
    I2C_SDA_SET();
    for(i = 0; i < 8; i++)
    {
        I2C_SCL_SET();
				SDA_IN();
        if(I2C_SDA_VALUE() == GPIO_PIN_SET)
            data |= (0x80 >> i);
        I2C_SCL_RESET();
    }SDA_OUT();
    
    return data;
}
void i2c_write_register(uint8_t i2c_address,uint8_t address, uint8_t data)
{
    //I2C起始信号
    i2c_start();
    //发送从机地址
    i2c_send_byte((i2c_address << 1) | 0x00);
    //等待从机响应
    i2c_receive_ack();
    //发送寄存器地址
    i2c_send_byte(address);
    //等待从机响应
    i2c_receive_ack();
    //发送写入的数据
    i2c_send_byte(data);
    //等待从机响应
    i2c_receive_ack();
    //I2C终止信号
    i2c_stop();
}

uint8_t i2c_read_register(uint8_t i2c_address,uint8_t address)
{
    uint8_t data = 0;
    
    //I2C起始信号
    i2c_start();
    //发送从机地址
    i2c_send_byte((i2c_address << 1) | 0x00);
    //等待从机响应
    i2c_receive_ack();
    //发送寄存器地址
    i2c_send_byte(address);
    //等待从机响应
    i2c_receive_ack();
    
    //I2C再次起始信号
    i2c_start();
    //发送从机地址
    i2c_send_byte((i2c_address << 1) | 0x01);
    //等待从机响应
    i2c_receive_ack();
    //接收从机数据
    data = i2c_receive_byte();
    //发送ack 1，停止数据传输
    i2c_send_ack(1);
    //I2C终止信号
    i2c_stop();
    
    return data;
}

void i2c_write_len(uint8_t i2c_address,uint8_t reg_addr, uint8_t len, uint8_t *data)
{
    uint8_t i = 0;
    
    //I2C起始信号
    i2c_start();
    //发送从机地址
    i2c_send_byte((i2c_address << 1) | 0x00);
    //等待从机响应
    i2c_receive_ack();
    //发送寄存器地址
    i2c_send_byte(reg_addr);
    //等待从机响应
    i2c_receive_ack();
    
    for(i = 0; i < len; i++)
    {
        //发送写入的数据
        i2c_send_byte(data[i]);
        //等待从机响应
        i2c_receive_ack();
    }
    
    //I2C终止信号
    i2c_stop();
}

void i2c_read_len(uint8_t i2c_address,uint8_t reg_addr, uint8_t len, uint8_t *data)
{
    //I2C起始信号
    i2c_start();
    //发送从机地址
    i2c_send_byte((i2c_address << 1) | 0x00);
    //等待从机响应
    i2c_receive_ack();
    //发送寄存器地址
    i2c_send_byte(reg_addr);
    //等待从机响应
    i2c_receive_ack();
    
    //I2C再次起始信号
    i2c_start();
    //发送从机地址
    i2c_send_byte((i2c_address << 1) | 0x01);
    //等待从机响应
    i2c_receive_ack();
    
    while(len--)
    {
        //接收从机数据
        *(data++) = i2c_receive_byte();
        if(len == 0)
            i2c_send_ack(1);
        else
            i2c_send_ack(0);
    }
    
    //I2C终止信号
    i2c_stop();
}


