#ifndef __DATA_H
#define __DATA_H
#include "sys.h"

// 机器人参数
#define LX 0.15f   // 轮子到中心的 X 方向距离（米）
#define LY 0.15f   // 轮子到中心的 Y 方向距离（米）
#define MAX_SPEED 100  // 最大速度（m/s 或 PWM 最大值）
// 核心常量（精简定义）

#define PI          3.1415926f
#define PI_2        (2 * PI)
#define THRESHOLD   (PI - 0.1f)
#define LPF_ALPHA 0.15f  // 滤波系数（0.05~0.3之间调，越小越平滑）

#define DEG2RAD(deg) ((deg) * M_PI / 180.0)
#define RAD2DEG(rad) ((rad) * 180.0 / M_PI)


extern uint8_t C_rx_buf[32];
extern uint8_t Uart_recv_byte[20];

extern float INS_angle_filtered[3];
extern float angle_lpf[3];
extern float last_angle[3];
extern float angle_offset[3];
extern float INS_continuous_angle[3];

extern float gyro[3];
extern float accel[3];
extern float mag[3];
extern float temp[2];
extern float INS_quat[4];
extern float INS_angle[3];


void gimbal_to_chassis_speed_compute(float gimbal_vx, float gimbal_vy, float yaw_rad,float *chassis_vx, float *chassis_vy);
void MecanumKinematics(int Vx, int Vy, int omega);
void OmniKinematics(int Vx, int Vy, int omega);
void float_to_two_uint8_signed(float num, uint8_t* high_byte, uint8_t* low_byte);
float two_uint8_to_float_signed(uint8_t high_byte, uint8_t low_byte);

void data_change(void);
void pid_calculate(void);
void data_print(void);

float process_continuous_angle(uint8_t axis, float curr);
float low_pass_filter(float input, float *prev_output) ;
uint8_t frame_sync(uint8_t byte, uint8_t *frame_out);
float normalizeAngleRad(float angle);
void data_init(void);
void uart_sand(void);
#endif


