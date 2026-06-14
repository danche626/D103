#include "Motor.h"
#include "delay.h"
#include "uart1.h"
#include "adc.h"

float shaft_angle=0,open_loop_timestamp=0;

void E_EN_init(void)
{
	GPIO_InitTypeDef gpio_initstruct;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	gpio_initstruct.Pin = GPIO_PIN_6;
	gpio_initstruct.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_initstruct.Pull = GPIO_PULLDOWN;
	gpio_initstruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA,&gpio_initstruct);
}
void M_EN_init(void)
{
	
	GPIO_InitTypeDef gpio_initstruct;

	__HAL_RCC_GPIOB_CLK_ENABLE();
	gpio_initstruct.Pin = GPIO_PIN_5;
	gpio_initstruct.Mode = GPIO_MODE_OUTPUT_PP;
	gpio_initstruct.Pull = GPIO_PULLDOWN;
	gpio_initstruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB,&gpio_initstruct);
}

void E_EN_open(void)
{HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_SET);}
void E_EN_close(void)
{HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_RESET);}

void M_EN_open(void)
{HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_SET);}
void M_EN_close(void)
{HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,GPIO_PIN_RESET);}



void Motor_init(void)
{
	
    float angle;

    for(int i=0; i<=500; i++)
	{
		angle = _3PI_2 + _2PI * i / 500.0;
		setPhaseVoltage(2.5, 0,  angle);
		delay_ms(2);
	}
	
	for(int i=500; i>=0; i--) 
	{
		angle = _3PI_2 + _2PI * i / 500.0 ;
		setPhaseVoltage(2.5, 0,  angle);
		delay_ms(2);
	}
	setPhaseVoltage(0, 0,  angle);
	delay_ms(100);
	setPhaseVoltage(3, 0,_3PI_2);
	delay_ms(300);
	zero_electric_angle=_electricalAngle();
  	setPhaseVoltage(0, 0,_3PI_2);
	printf("%f\n",zero_electric_angle);

}

//开环速度控制
float velocityOpenloop(float target_velocity)
{

	unsigned long now_us;
	float Ts,Uq;
	
	now_us = SysTick->VAL; //读取滴答定时器的值
	
	if(now_us<open_loop_timestamp)Ts = (float)(open_loop_timestamp - now_us)/9*1e-6;//转化为时间
	else
		Ts = (float)(0xFFFFFF - now_us + open_loop_timestamp)/9*1e-6;
	open_loop_timestamp=now_us;  //储存这次的值

    if(Ts == 0 || Ts > 0.5) Ts = 1e-3; 
	//printf("%.2f\n",Ts);

  shaft_angle = _normalizeAngle(shaft_angle + target_velocity*Ts); 
	//printf("%.2f\n",shaft_angle);
    Uq = 6;

	setPhaseVoltage(Uq,  0, shaft_angle*pole_pairs);
	return Uq;
	
}

//位置闭环
void PID_Pos_Set(float P, float I, float D, float limit)
{
    PID_Pos.P = P;
    PID_Pos.I = I;
    PID_Pos.D = D;
    PID_Pos.limit = limit;
}
void PositionCloseloop(float position)
{
    float output;
    output=PID_operator(&PID_Pos,(position-sensor_direction*Get_Angel())*180/PI);
    setPhaseVoltage(output,0,_electricalAngle());
}

//速度闭环
void PID_Vel_Set(float P, float I, float D, float limit)
{
    PID_Vel.P = P;
    PID_Vel.I = I;
    PID_Vel.D = D;
    PID_Vel.limit = limit;
}

void VelocityCloseloop(float Velocity)
{
    float output;
    output=PID_operator(&PID_Vel,(Velocity-sensor_direction*Get_Velocity())*180/PI);
//	printf("%.2f\r\n",output);
    setPhaseVoltage(output,0,_electricalAngle());
    
}

//电流环

void PID_Cur_Set(float P, float I, float D, float limit)
{
    PID_Cur.P = P;
    PID_Cur.I = I;
    PID_Cur.D = D;
    PID_Cur.limit = limit;
}

void CurrentCloseloop(float current)
{
    float output;
    output=PID_operator(&PID_Cur,(current-Get_Current()));
    setPhaseVoltage(output,0,_electricalAngle());
	
}

//位置速度闭环

void Position_VelocityCloseloop(float position)
{
    float output;
    output=PID_operator(&PID_Pos,(position-sensor_direction*Get_Angel())*180/PI);
	VelocityCloseloop(output);
}

//速度电流闭环
void Velocity_CurrentCloseloop(float velocity)
{
    float output;
    output=PID_operator(&PID_Vel,(velocity-sensor_direction*Get_Velocity())*180/PI);
	CurrentCloseloop(output);
}

//位置电流闭环
void Position_CurrentCloseloop(float position)
{
    float output;
    output=PID_operator(&PID_Pos,(position-sensor_direction*Get_Angel())*180/PI);
    CurrentCloseloop(output);
}


