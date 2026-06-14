#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "malloc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bmi088.h"
#include "bmi055.h"
#include "ist8310.h"
#include "imu.h"
#include "spi.h"
#include "oled.h"
#include "nrf24l01.h"
#include "pid.h"
#include "my_task.h"
#include "iic_hw.h"
#include "data.h"
#include "motor.h"

int main(void)
{
    HAL_Init();                         
	sys_stm32_clock_init(336, 8, 2, 7); 
    delay_init(168);                    
    usart_init(115200);                 
    led_init(); 

	LED3(1);	
	
    key_init();                         
    motor_init();               
	pwm_init(16799, 0);
	spi2_init();
	iic_init();
	nrf24l01_init();
	
	BMI055_init();
	imu_init();
	
	NRF_check();
	data_init();


	create_task();
	vTaskStartScheduler();

}
