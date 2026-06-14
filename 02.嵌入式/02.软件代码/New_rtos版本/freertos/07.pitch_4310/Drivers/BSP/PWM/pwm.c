#include "pwm.h"

TIM_HandleTypeDef pwm1_handle = {0};			
TIM_HandleTypeDef pwm2_handle = {0};

void pwm_init(uint16_t arr,uint16_t psc)
{
		TIM_OC_InitTypeDef pwm1_config = {0};
		TIM_OC_InitTypeDef pwm2_config = {0};
		
		pwm1_handle.Instance = TIM1;			
		pwm1_handle.Init.Prescaler = psc;
		pwm1_handle.Init.Period = arr;
		pwm1_handle.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
		
		HAL_TIM_PWM_Init(&pwm1_handle);
		
		pwm1_config.OCMode = TIM_OCMODE_PWM1;
		pwm1_config.Pulse  = arr/2;
		pwm1_config.OCPolarity = TIM_OCPOLARITY_HIGH;
		
		pwm2_handle.Instance = TIM4;			
		pwm2_handle.Init.Prescaler = psc;
		pwm2_handle.Init.Period = arr;
		pwm2_handle.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
		
		HAL_TIM_PWM_Init(&pwm2_handle);
		
		pwm2_config.OCMode = TIM_OCMODE_PWM1;
		pwm2_config.Pulse  = arr/2;
		pwm2_config.OCPolarity = TIM_OCPOLARITY_HIGH;
		
		HAL_TIM_PWM_ConfigChannel(&pwm1_handle,&pwm1_config,TIM_CHANNEL_1);
		HAL_TIM_PWM_ConfigChannel(&pwm1_handle,&pwm1_config,TIM_CHANNEL_2);
		HAL_TIM_PWM_ConfigChannel(&pwm1_handle,&pwm1_config,TIM_CHANNEL_3);

		HAL_TIM_PWM_ConfigChannel(&pwm2_handle,&pwm2_config,TIM_CHANNEL_1);
		HAL_TIM_PWM_ConfigChannel(&pwm2_handle,&pwm2_config,TIM_CHANNEL_2);
		HAL_TIM_PWM_ConfigChannel(&pwm2_handle,&pwm2_config,TIM_CHANNEL_3);
		
		HAL_TIM_PWM_Stop(&pwm1_handle,TIM_CHANNEL_1);
		HAL_TIM_PWM_Stop(&pwm1_handle,TIM_CHANNEL_2);
		HAL_TIM_PWM_Stop(&pwm1_handle,TIM_CHANNEL_3);
		
		HAL_TIM_PWM_Stop(&pwm2_handle,TIM_CHANNEL_1);
		HAL_TIM_PWM_Stop(&pwm2_handle,TIM_CHANNEL_2);
		HAL_TIM_PWM_Stop(&pwm2_handle,TIM_CHANNEL_3);
}
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
		if(htim->Instance == TIM1){
			GPIO_InitTypeDef gpio_initstruct;
			
			__HAL_RCC_TIM1_CLK_ENABLE();		
			__HAL_RCC_GPIOA_CLK_ENABLE();
	
			gpio_initstruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
			gpio_initstruct.Mode = GPIO_MODE_AF_PP;
			gpio_initstruct.Pull = GPIO_PULLUP;
			gpio_initstruct.Speed = GPIO_SPEED_FREQ_HIGH;
	
			HAL_GPIO_Init(GPIOA,&gpio_initstruct);		
		}
		if(htim->Instance == TIM4){
			GPIO_InitTypeDef gpio2_initstruct;
			
			__HAL_RCC_TIM4_CLK_ENABLE();		
			__HAL_RCC_GPIOB_CLK_ENABLE();
	
			gpio2_initstruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8;
			gpio2_initstruct.Mode = GPIO_MODE_AF_PP;
			gpio2_initstruct.Pull = GPIO_PULLUP;
			gpio2_initstruct.Speed = GPIO_SPEED_FREQ_HIGH;
	
			HAL_GPIO_Init(GPIOB,&gpio2_initstruct);		
		}
}
void motor1_pwm_set(uint16_t val1,uint16_t val2,uint16_t val3)
{
	HAL_TIM_PWM_Start(&pwm1_handle,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&pwm1_handle,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&pwm1_handle,TIM_CHANNEL_3);
	
	__HAL_TIM_SET_COMPARE(&pwm1_handle,TIM_CHANNEL_1,val1);
	__HAL_TIM_SET_COMPARE(&pwm1_handle,TIM_CHANNEL_2,val2);
	__HAL_TIM_SET_COMPARE(&pwm1_handle,TIM_CHANNEL_3,val3);
}
void motor2_pwm_set(uint16_t val1,uint16_t val2,uint16_t val3)
{
	HAL_TIM_PWM_Start(&pwm2_handle,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&pwm2_handle,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&pwm2_handle,TIM_CHANNEL_3);
	
	__HAL_TIM_SET_COMPARE(&pwm2_handle,TIM_CHANNEL_1,val1);
	__HAL_TIM_SET_COMPARE(&pwm2_handle,TIM_CHANNEL_2,val2);
	__HAL_TIM_SET_COMPARE(&pwm2_handle,TIM_CHANNEL_3,val3);
}
void pwm_compare_set(uint16_t val1,uint16_t val2,uint16_t val3)
{
	__HAL_TIM_SET_COMPARE(&pwm1_handle,TIM_CHANNEL_1,val1);
	__HAL_TIM_SET_COMPARE(&pwm1_handle,TIM_CHANNEL_2,val2);
	__HAL_TIM_SET_COMPARE(&pwm1_handle,TIM_CHANNEL_3,val3);
	
	__HAL_TIM_SET_COMPARE(&pwm2_handle,TIM_CHANNEL_1,val1);
	__HAL_TIM_SET_COMPARE(&pwm2_handle,TIM_CHANNEL_2,val2);
	__HAL_TIM_SET_COMPARE(&pwm2_handle,TIM_CHANNEL_3,val3);
	
	HAL_TIM_PWM_Start(&pwm1_handle,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&pwm1_handle,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&pwm1_handle,TIM_CHANNEL_3);	
	
	HAL_TIM_PWM_Start(&pwm2_handle,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&pwm2_handle,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&pwm2_handle,TIM_CHANNEL_3);
}
