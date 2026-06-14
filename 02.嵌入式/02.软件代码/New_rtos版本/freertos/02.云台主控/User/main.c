#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "malloc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bmi055.h"
#include "ist8310.h"
#include "imu.h"
#include "spi.h"
#include "laser.h"
#include "oled.h"
#include "nrf24l01.h"
#include "pid.h"
#include "lvbo.h"
#include "my_task.h"
#include "iic_hw.h"
#include "data.h"
#include "ws2812b.h"

int main(void)
{
    HAL_Init();                         /* 初始化HAL库 */
	sys_stm32_clock_init(336, 8, 2, 7); /* 设置时钟,168Mhz */
    delay_init(168);                    /* 延时初始化 */
    usart_init(115200);                 /* 串口初始化为115200 */
    led_init();                         /* 初始化LED */
    key_init();                         /* 初始化按键 */	
    spi2_init();
	iic_init();
	nrf24l01_init();
	BMI055_init();
	ist8310_init();
	imu_init();
	NRF_check();
	data_init();

	printf("runing......\r\n");
	
    create_task();
	vTaskStartScheduler();

}
