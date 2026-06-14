#ifndef __IIC_H__
#define __IIC_H__
#include "uart1.h"
#include "delay.h"

#define I2C_SCL_CLK()      __HAL_RCC_GPIOB_CLK_ENABLE();
#define I2C_SCL_PORT       GPIOB
//#define I2C_SCL1_PIN        GPIO_PIN_6
#define I2C_SCL2_PIN        GPIO_PIN_10

#define I2C_SDA_CLK()      __HAL_RCC_GPIOB_CLK_ENABLE();
#define I2C_SDA_PORT       GPIOB
//#define I2C_SDA1_PIN        GPIO_PIN_7
#define I2C_SDA2_PIN        GPIO_PIN_11

#define I2C_SCL_RESET()        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL2_PIN, GPIO_PIN_RESET)
#define I2C_SCL_SET()          HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL2_PIN, GPIO_PIN_SET)

#define I2C_SDA_RESET()        HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA2_PIN, GPIO_PIN_RESET)
#define I2C_SDA_SET()          HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA2_PIN, GPIO_PIN_SET)

#define I2C_SDA_VALUE()         HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA2_PIN)

// 设置SDA为输入模式:cite[3]
#define SDA_IN() do { \
  GPIO_InitTypeDef GPIO_InitStruct = {0}; \
  GPIO_InitStruct.Pin = I2C_SDA2_PIN; \
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT; \
  GPIO_InitStruct.Pull = GPIO_PULLUP; \
  HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct); \
} while (0)

// 设置SDA为输出模式:cite[3]
#define SDA_OUT() do { \
  GPIO_InitTypeDef GPIO_InitStruct = {0}; \
  GPIO_InitStruct.Pin = I2C_SDA2_PIN; \
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; \
  GPIO_InitStruct.Pull = GPIO_PULLUP; \
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; \
  HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct); \
} while (0)

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_send_byte(uint8_t data);
void i2c_send_ack(uint8_t ack);
uint8_t i2c_receive_byte(void);
uint8_t i2c_receive_ack(void);	
void i2c_write_register(uint8_t i2c_address,uint8_t address, uint8_t data);
uint8_t i2c_read_register(uint8_t i2c_address,uint8_t address);
void i2c_read_len(uint8_t i2c_address,uint8_t reg_addr, uint8_t len, uint8_t *data);

#endif

