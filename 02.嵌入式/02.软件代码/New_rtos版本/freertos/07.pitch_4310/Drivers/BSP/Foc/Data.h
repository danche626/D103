#ifndef DATA_H
#define DATA_H

#include "Pid.h"

#define PI 3.1415926f
#define _3PI_2 4.71238898038f
#define _PI_3 1.0471975512f
#define _PI_2 1.57079632679f
#define _2PI 6.28318530718f
#define	_1_SQRT3 0.57735026919f
#define _2_SQRT3 1.1547005383f

extern float yaw_num;
extern float joy_num;
extern float yaw_pid_num;

extern uint8_t uart_recv[20];
extern uint8_t send_angal[20]; 

extern float sensor_direction;//编码器方向
extern float pole_pairs;//电机极对数
extern float voltage_power_supply ;//供电电压
extern float Ualpha,Ubeta,Ua,Ub,Uc;
extern float zero_electric_angle;

extern uint16_t adc_result[4];
extern PID PID_Pos;//位置PID
extern PID PID_Vel;//速度PID
extern PID PID_Cur;//电流PID
extern float position;//位置
extern float velocity;//速度
extern float current;//电流

extern float gain_a,gain_b,gain_c;//欧姆定律系数
extern float offset_ia,offset_ib,offset_ic;//采集的3路电流
extern float volts_to_amps_ratio;
extern float shunt_resistor ;//采样电阻值
extern float amp_gain;//运放增益
extern float current_a,current_b;//帕克变换的值

void float_to_two_uint8_signed(float num, uint8_t* high_byte, uint8_t* low_byte);
float two_uint8_to_float_signed(uint8_t high_byte, uint8_t low_byte);
uint8_t find_frame(const uint8_t *buf, uint16_t len, uint8_t *out);
uint8_t frame_sync(uint8_t byte, uint8_t *frame_out);
float lowpass_filter_float(float new_val, float alpha);
void data_change(void);
void data_print(void);
void data_init(void);
#endif 
