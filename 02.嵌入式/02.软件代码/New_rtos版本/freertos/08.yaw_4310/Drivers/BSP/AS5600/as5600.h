#ifndef  __AS5600_H__
#define  __AS5600_H__
#include "sys.h"
#include "Data.h"

typedef struct 
{
	long  velocity_calc_timestamp;  //速度计时，用于计算速度
	long  cpr;                      //编码器分辨率，AS5600=12bit(4096)
	long  angle_data_prev;          //获取角度用
	float angle_prev;               //获取速度用
	float full_rotation_offset;     //角度累加
} ENCODER_TypeDef;


void as5600_init(void);
float as5600_read_angal(void);
float Get_Angel(void);
float Get_Velocity2(void);
float Get_Velocity(void);
float Get_Angel_Notrack(void);
float sliding_avg_filter(float new_val);
#endif
