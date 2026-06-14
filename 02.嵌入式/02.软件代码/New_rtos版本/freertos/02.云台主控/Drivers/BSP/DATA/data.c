#include "data.h"
#include "math.h"
#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "spi.h"
#include "laser.h"
#include "oled.h"
#include "nrf24l01.h"
#include "bmi055.h"
#include "ist8310.h"
#include "imu.h"
#include "pid.h"
#include "lvbo.h"
#include "timer.h"
#include "iic_hw.h"

// 核心常量（精简定义）
#define PI          3.1415926f
#define PI_2        (2 * PI)
#define THRESHOLD   (PI - 0.1f)
#define LPF_ALPHA 0.15f  // 滤波系数（0.05~0.3之间调，越小越平滑）
#define LIMIT(val, min, max) (((val) < (min)) ? (min) : (((val) > (max)) ? (max) : (val)))

/*********imu相关*********/
float last_angle[3] = {0.0f, 0.0f, 0.0f};
float INS_angle_filtered[3] = {0.0f, 0.0f, 0.0f};
float angle_offset[3] = {0.0f, 0.0f, 0.0f};
float INS_continuous_angle[3] = {0.0f, 0.0f, 0.0f};
float angle_lpf[3] = {0.0f, 0.0f, 0.0f};   // 连续角度滤波
float gyro[3];
float accel[3];
float mag[3];
float temp[2];
float INS_quat[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float INS_angle[3] = {0.0f, 0.0f, 0.0f};    
/*************************/
int8_t joy[5] = {0};
uint8_t B_rx_A_buf[32] = {0};
uint8_t Uart_sand_byte[20] = {0};
uint8_t Uart_recv_byte[20] = {0};
pid_type_def pitch_pos_pid;

float pitch_position[3] = {0.3f, 0.0f, 0.01f};
float pitch_val[3] = {0.0f,0.0f,0.0f};				//接收遥控器值


void data_init(void)
{
	PID_init(&pitch_pos_pid, PID_POSITION, pitch_position, 300.0f, 100.0f);
}

void pid_calculate(void)
{
		pitch_val[1] = PID_calc(&pitch_pos_pid, -pitch_val[0], INS_angle_filtered[1]);
}

void data_change(void)
{
		// 串口dma接收
		usart_poll();
	
		// 拷贝数据
		for(int i=0;i<20;i++){Uart_sand_byte[i] = B_rx_A_buf[i];}; 
		
		// 需要发送的值 遥控器值 pitch轴值 pitch pid值
		float_to_two_uint8_signed(INS_angle_filtered[1],&Uart_sand_byte[12],&Uart_sand_byte[13]);		//pitch
		float_to_two_uint8_signed(pitch_val[1],&Uart_sand_byte[14],&Uart_sand_byte[15]);				//pitch_pid

		// 数据解算
		joy[0] = B_rx_A_buf[1];//按键值
		joy[1] = B_rx_A_buf[2];//编码器值
		joy[2] = B_rx_A_buf[3];//编码器值 
		pitch_val[0] = two_uint8_to_float_signed(B_rx_A_buf[6],B_rx_A_buf[7]);							//lY摇杆值
		pitch_val[2] = normalizeAngleRad(two_uint8_to_float_signed(Uart_recv_byte[1],Uart_recv_byte[2]));
}
void uart_sand(void)
{
	HAL_UART_Transmit(&g_uart1_handle,Uart_sand_byte,sizeof(Uart_sand_byte),1000);
}
void data_print(void)
{

//	printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",target_val[0],target_val[1],INS_angle_filtered[0],INS_angle_filtered[1],output_val[1],output_val[3]);
	
//	printf("%.2f,%.2f,%.2f\n",accel[0],accel[1],accel[2]);
//	printf("%.2f,%.2f,%.2f\n",gyro[0],gyro[1],gyro[2]);
//	printf("%.2f,%.2f,%.2f\n",mag[0],mag[1],mag[2]);
//	printf("%.2f,%.2f\n",temp[0],temp[1]);
//	printf("%.2f,%.2f,%.2f,%.2f\n",INS_quat[0],INS_quat[1],INS_quat[2],INS_quat[3]);
//	printf("%.2f,%.2f,%.2f,%.2f\n",output_val[0],output_val[1],output_val[2],output_val[3]);
	
//	printf("%.2f\r\n",pitch_val[2]);
	
		for(int i=0;i<20;i++){
			printf("%2x,",Uart_sand_byte[i]);
		}printf("\r\n");
		
//		printf("%.2f,%.2f,%.2f\n",INS_angle[0],INS_angle[1],INS_angle[2]);	

}

/**
  * @brief  带符号浮点数转两个uint8_t（范围-327.68~327.67，两位小数）
  * @param  num: 输入浮点数（范围-327.68~327.67）
  * @param  high_byte: 高位字节输出
  * @param  low_byte: 低位字节输出
  * @note   精度0.01，有效范围-327.68~327.67
  */
void float_to_two_uint8_signed(float num, uint8_t* high_byte, uint8_t* low_byte) 
{
    // 范围检查（可选）
    if(num > 327.67f) num = 327.67f;
    if(num < -327.68f) num = -327.68f;
    
    // 放大100倍并四舍五入
    int16_t scaled = (int16_t)(round(num * 100.0f));
    
    // 将int16_t拆分为两个字节
    *high_byte = (scaled >> 8) & 0xFF;
    *low_byte = scaled & 0xFF;
}

/**
  * @brief  两个uint8_t还原为带符号浮点数
  * @param  high_byte: 高位字节
  * @param  low_byte: 低位字节
  * @retval 还原的浮点数（范围-327.68~327.67）
  */
float two_uint8_to_float_signed(uint8_t high_byte, uint8_t low_byte) 
{
    // 组合为int16_t（注意：高位需要符号扩展）
    int16_t scaled = (int16_t)((high_byte << 8) | low_byte);
    
    // 缩小100倍
    return (float)scaled / 100.0f;
}
	float process_continuous_angle(uint8_t axis, float curr) {
    float delta = curr - last_angle[axis];
    if(delta < -THRESHOLD) angle_offset[axis] += PI_2;
    else if(delta > THRESHOLD) angle_offset[axis] -= PI_2;
    last_angle[axis] = curr;
    return curr + angle_offset[axis];
}

/**
 * 单轴一阶低通滤波函数
 * @param input: 当前采样值 | @param prev_output: 上次滤波输出（指针）
 * @return: 本次滤波输出
 */
float low_pass_filter(float input, float *prev_output) 
{
    *prev_output = LPF_ALPHA * input + (1.0f - LPF_ALPHA) * (*prev_output);
    return *prev_output;
}
uint8_t frame_sync(uint8_t byte, uint8_t *frame_out)
{
    static uint8_t ring[20];
    static uint8_t w_idx = 0;
    static uint8_t count = 0;

    ring[w_idx] = byte;
    w_idx = (w_idx + 1) % 20;
    if (count < 20) count++;

    if (count >= 20) {
        uint8_t start = w_idx;
        if (ring[start] == 0x55 && ring[(start + 19) % 20] == 0xFF) {
            for (uint8_t i = 0; i < 20; i++) {
                frame_out[i] = ring[(start + i) % 20];
            }
            count = 0;   // 清空计数器，避免重复输出同一帧
            return 1;
        }
    }
    return 0;
}
// 角度归一化到 -2PI - 2PI
double normalizeAngleRad(double angle)
{
    const double two_pi = 2 * PI;
    return fmod(angle, two_pi);
}
