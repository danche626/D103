#include "sys.h"
#include "uart1.h"
#include "delay.h"
#include "LED.h"
#include "iic.h"
#include "key.h"
#include "oled.h"
#include "nrf24L01.h"
#include "as5600.h"
#include "Data.h"
#include "FOC.h"
#include "pwm.h"
#include "Lowpass.h"
#include "Motor.h"
#include "Pid.h"
#include "adc.h"
#include "can.h"
#include "dma.h" 

int main(void)
{
    HAL_Init();                                                               
    stm32_clock_init(RCC_PLL_MUL9); 
	
	lde_init();
	i2c_init();
	key_init();
	uart1_init(115200);

	pwm_init(1440,0);
	as5600_init();	
	dma_init();
	
	lde1_open();

			
	PID_Pos_Set(1.0,0.0,0.0,62.8);
	PID_Vel_Set(0.01,0.0,0.0,60);
	PID_Cur_Set(0.5,0,0.0,6);
	data_init();
	
	CurrSense_init();
	LOWPass_Init();
	Motor_init();
 
	HAL_UART_Transmit_DMA(&uart1_handle, send_angal, sizeof(send_angal));
	
    while(1)
    {
		data_change();
//		data_print();
	}

}

