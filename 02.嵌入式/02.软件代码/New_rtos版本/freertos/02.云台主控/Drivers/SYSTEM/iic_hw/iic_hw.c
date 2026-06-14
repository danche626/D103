#include "iic_hw.h"
#include "delay.h"

/* I2C句柄定义 */
I2C_HandleTypeDef hi2c2;

/**
 * @brief 手动配置GPIO引脚（硬件IIC专用）
 * @note  PB10->I2C2_SCL, PB11->I2C2_SDA
 */
static void IIC_HW_GPIO_Config(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* 1. 使能GPIO时钟 */
    IIC_HW_GPIO_CLK_ENABLE();

    /* 2. 配置SCL引脚：复用开漏 + 上拉 + 高速 */
    gpio_init_struct.Pin = IIC_HW_SCL_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_OD;          // 硬件IIC必须开漏复用
    gpio_init_struct.Pull = GPIO_PULLUP;              // 上拉（IIC总线必备）
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = GPIO_AF4_I2C2;       // PB10复用为I2C2_SCL（查STM32手册）
    HAL_GPIO_Init(IIC_HW_GPIO_PORT, &gpio_init_struct);

    /* 3. 配置SDA引脚：复用开漏 + 上拉 + 高速 */
    gpio_init_struct.Pin = IIC_HW_SDA_PIN;
    gpio_init_struct.Alternate = GPIO_AF4_I2C2;       // PB11复用为I2C2_SDA
    HAL_GPIO_Init(IIC_HW_GPIO_PORT, &gpio_init_struct);
}

/**
 * @brief 手动配置I2C外设寄存器（HAL库底层）
 */
static void IIC_HW_I2C_Config(void)
{
    /* 1. 使能I2C时钟 */
    IIC_HW_CLK_ENABLE();

    /* 2. 初始化I2C句柄 */
    hi2c2.Instance = IIC_HW_I2Cx;
    hi2c2.Init.ClockSpeed = 400000;                  // IIC时钟频率400KHz（快速模式）
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;           // 占空比2:1
    hi2c2.Init.OwnAddress1 = 0x00;                    // 主机模式，自身地址设为0
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT; // 7位地址模式
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0x00;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    /* 3. 初始化I2C外设（HAL库核心函数） */
    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    {
        /* 初始化失败处理（可根据需求加断言/重启） */
        while (1);
    }
}

/**
 * @brief 硬件IIC初始化（替换原模拟IIC的iic_init函数）
 */
void iic_init(void)
{
    IIC_HW_GPIO_Config();  // 配置GPIO
    IIC_HW_I2C_Config();   // 配置I2C外设
}

/**
 * @brief 硬件IIC写入单个寄存器（兼容原函数名）
 * @param i2c_addr: I2C设备7位地址（无需左移）
 * @param reg_addr: 寄存器地址
 * @param data: 要写入的数据
 */
void iic_write_register(uint8_t i2c_addr, uint8_t reg_addr, uint8_t data)
{
    taskENTER_CRITICAL();  // FreeRTOS临界区保护

    /* HAL库硬件IIC写寄存器：核心函数 */
    HAL_I2C_Mem_Write(&hi2c2,
                      (i2c_addr << 1),  // 7位地址左移1位（HAL自动补写位0）
                      reg_addr,
                      I2C_MEMADD_SIZE_8BIT,  // 寄存器地址8位
                      &data,
                      1,
                      IIC_HW_TIMEOUT);

    taskEXIT_CRITICAL();
}

/**
 * @brief 硬件IIC读取单个寄存器（兼容原函数名）
 * @param i2c_addr: I2C设备7位地址
 * @param reg_addr: 寄存器地址
 * @retval 读取到的数据（失败返回0xFF）
 */
uint8_t iic_read_register(uint8_t i2c_addr, uint8_t reg_addr)
{
    uint8_t data = 0xFF;
    taskENTER_CRITICAL();

    /* HAL库硬件IIC读寄存器：核心函数 */
    if (HAL_I2C_Mem_Read(&hi2c2,
                         (i2c_addr << 1),  // 7位地址左移1位（HAL自动补读位1）
                         reg_addr,
                         I2C_MEMADD_SIZE_8BIT,
                         &data,
                         1,
                         IIC_HW_TIMEOUT) != HAL_OK)
    {
        data = 0xFF;  // 读取失败返回0xFF（兼容原模拟IIC逻辑）
    }

    taskEXIT_CRITICAL();
    return data;
}

/**
 * @brief 硬件IIC读取多字节（兼容原函数名）
 * @param i2c_addr: I2C设备7位地址
 * @param reg_addr: 起始寄存器地址
 * @param buf: 数据缓冲区
 * @param len: 读取长度
 * @retval 0:成功, 1:失败（兼容原模拟IIC返回值）
 */
uint8_t iic_read_len(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    uint8_t ret = 1;  // 默认失败
    taskENTER_CRITICAL();

    if (HAL_I2C_Mem_Read(&hi2c2,
                         (i2c_addr << 1),
                         reg_addr,
                         I2C_MEMADD_SIZE_8BIT,
                         buf,
                         len,
                         IIC_HW_TIMEOUT) == HAL_OK)
    {
        ret = 0;  // 读取成功
    }

    taskEXIT_CRITICAL();
    return ret;
}

/************************ HAL库底层回调（可选） ************************/
/**
 * @brief I2C错误回调函数（调试用）
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == IIC_HW_I2Cx)
    {
        /* 可添加错误处理：重启IIC、打印日志等 */
        HAL_I2C_DeInit(hi2c);
        IIC_HW_I2C_Config();
    }
}

