#include "as5600.h"
#include "iic.h"
#include "math.h"
#include "Lowpass.h"
#include "Data.h"

void as5600_init(void)	
{   i2c_write_register(0x36, 0x07, 0x00);
    i2c_write_register(0x36, 0x08, 0x00);
	
//	uint8_t data = i2c_read_register(0x36,0x00);
//	printf("data = 0x%x\r\n",data);
	
}
float as5600_read_angal(void)
{
	// 读取角度（12位值）
    uint8_t angle_high = i2c_read_register(0x36, 0x0C);
    uint8_t angle_low = i2c_read_register(0x36, 0x0D);
	
    uint16_t raw_angle = ((angle_high & 0x0F) << 8) | angle_low;
	
//	return (raw_angle / 4096.0) * 360.0;   
//  return (raw_angle / 4096.0)* 6.28318530717958647692f;
	return raw_angle;
} 
ENCODER_TypeDef E1;

float Get_Angel_L(ENCODER_TypeDef *CODERx)
{
    float angle_data=0,d_angle=0;
	
    CODERx->cpr=4096;
	angle_data =  as5600_read_angal();
	

	d_angle = angle_data - CODERx->angle_data_prev;

 	if(fabs(d_angle) > (0.8*CODERx->cpr) ) CODERx->full_rotation_offset += d_angle > 0 ? -_2PI : _2PI; 

	CODERx->angle_data_prev = angle_data;

	return  (CODERx->full_rotation_offset + ( angle_data / (float)CODERx->cpr) * _2PI) ;
}

float Get_Angel(void)
{
	return Get_Angel_L(&E1);
}


ENCODER_TypeDef E2;

float Get_Velocity_L(ENCODER_TypeDef* CODERx)
{
	unsigned long now_us;
	float Ts, angle_c, vel;

	// calculate sample time
	now_us = SysTick->VAL; //_micros();
	if(now_us<CODERx->velocity_calc_timestamp)Ts = (float)(CODERx->velocity_calc_timestamp - now_us)/9*1e-6;
	else
		Ts = (float)(0xFFFFFF - now_us + CODERx->velocity_calc_timestamp)/9*1e-6;
	// quick fix for strange cases (micros overflow)
	if(Ts == 0 || Ts > 0.5) Ts = 1e-3;

	// current angle
	angle_c = Get_Angel();
	// velocity calculation
	vel = (angle_c - CODERx->angle_prev)/Ts;

	// save variables for future pass
	CODERx->angle_prev = angle_c;
	CODERx->velocity_calc_timestamp = now_us;
	
	return vel;
}
float Get_Velocity_H(ENCODER_TypeDef* CODERx)
{
    uint32_t now_count, delta_count;
    float Ts, angle_diff, vel;
    
    // 获取当前计数值（24位递减计数器）
    now_count = SysTick->VAL & 0xFFFFFF;
    
    // 计算时间间隔（考虑递减和重载）
    if(now_count < CODERx->velocity_calc_timestamp) {
        // 正常递减，没有发生重载
        delta_count = CODERx->velocity_calc_timestamp - now_count;
    } else {
        // 发生重载：从上次值到0，然后从重载值到现在
        delta_count = CODERx->velocity_calc_timestamp + (0xFFFFFF - now_count);
    }
    
    // 将计数间隔转换为时间（秒）
    // 假设SysTick设置为系统时钟的8分频，则每个计数的时间为 1/(SystemCoreClock/8)
    // 这里需要根据实际系统时钟和分频设置调整
    // 例如，系统时钟为72MHz，8分频后为9MHz，每个计数为1/9e6秒
    Ts = (float)delta_count / (SystemCoreClock / 8);  // 注意：这里需要确认分频系数
    
    // 保护：如果时间间隔异常，使用上次的时间间隔或默认值
    if(Ts <= 0 || Ts > 0.5f) {
        Ts = 1e-3f;  // 1ms
    }
    
    // 获取当前角度
    float angle_c = Get_Angel();  // 注意：函数名拼写错误，应为Get_Angle
    
    // 计算角度差，并处理360°溢出
    angle_diff = angle_c - CODERx->angle_prev;
    // 将角度差限制在[-180, 180]度范围内
    if(angle_diff > 180.0f) {
        angle_diff -= 360.0f;
    } else if(angle_diff < -180.0f) {
        angle_diff += 360.0f;
    }
    
    // 计算速度（度/秒）
    vel = angle_diff / Ts;
    
    // 保存当前角度和时间戳
    CODERx->angle_prev = angle_c;
    CODERx->velocity_calc_timestamp = now_count;
    
    return vel;
}
//无滤波速度
float Get_Velocity2(void)
{
	return Get_Velocity_L(&E2);

}
//有滤波速度
float Get_Velocity(void)
{
	float vel_ori=Get_Velocity_H(&E2);

  	float vel_flit=LowPassFilter_operator(&LPF_velocity,vel_ori);
  	return vel_flit;   
}
float Get_Angel_Notrack(void)
{
    return  as5600_read_angal()*0.08789*PI/180;
}

