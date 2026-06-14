#include "data.h"
#include "math.h"
#include "sys.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "key.h"
#include "nrf24l01.h"
#include "bmi088.h"
#include "ist8310.h"
#include "imu.h"
#include "pid.h"
#include "iic.h"
#include "motor.h"
#include "pid.h"

#define data_kp 120

int8_t joy[5] = {0};
float joy_yaw_num = 0.0f;
int wheel_speeds[4];

uint8_t C_rx_buf[32] = {0};
uint8_t Uart_sand_byte[20] = {0};
uint8_t Uart_recv_byte[20] = {0};

pid_type_def yaw_gimble_pid;
pid_type_def yaw_chassis_pid;

float yaw_gimble_pos[3]   	  = {1.0f, 0.0f, 0.01f};	//小陀螺
float yaw_chassis_pos[3] 	  = {200.0f, 0.0f, 0.0f};	//底盘跟随
float yaw_output[2] 		  = {0.0f,0.0f};			//pid输出
float gimble_chassis[2] 	  = {0.0f,0.0f};			//小陀螺速度输出

float gimble_yaw = 0.0f;//编码器值
float gimble_angal = 0.5f;//角度偏差
float normal_yaw = 0.0f;//角度偏差
//imu相关
float last_angle[3]           = {0.0f, 0.0f, 0.0f};
float INS_angle_filtered[3]   = {0.0f, 0.0f, 0.0f};
float angle_offset[3]         = {0.0f, 0.0f, 0.0f};
float INS_continuous_angle[3] = {0.0f, 0.0f, 0.0f};
float angle_lpf[3]            = {0.0f, 0.0f, 0.0f};   
float gyro[3];
float accel[3];
float mag[3];
float temp[2];
float INS_quat[4]             = {0.0f, 0.0f, 0.0f, 0.0f};
float INS_angle[3] 			  = {0.0f, 0.0f, 0.0f};


void data_init(void)
{
	PID_init(&yaw_gimble_pid, PID_POSITION, yaw_gimble_pos,   300.0f, 300.0f);//小陀螺
	PID_init(&yaw_chassis_pid, PID_POSITION, yaw_chassis_pos, 300.0f, 300.0f);//底盘跟随

}

void pid_calculate(void)
{
		yaw_output[0] = PID_calc(&yaw_gimble_pid, joy_yaw_num, INS_angle_filtered[0]);      //小陀螺pid输出  目标值遥控器值 反馈值陀螺仪角度
		yaw_output[1] = PID_calc(&yaw_chassis_pid, gimble_angal ,normal_yaw);  //底盘跟随pid输出 目标值遥控器值 反馈值yaw编码器值
}
/*
	帧格式
	0x55 按键 enl enr lxh lxl lyh lyl
	rxh  rxl  ryh ryl yaw yaw pid pid
	0    0    0   0xff
*/
void data_change(void)
{
		// 串口dma接收
		usart_poll();
	
		// 拷贝数据
		for(int i=0;i<20;i++){Uart_sand_byte[i] = C_rx_buf[i];}; 
		
		// 需要发送的值 遥控器值 yaw轴值 yaw pid值
		float_to_two_uint8_signed(INS_angle_filtered[0],&Uart_sand_byte[12],&Uart_sand_byte[13]);		//yaw
		float_to_two_uint8_signed(yaw_output[0],&Uart_sand_byte[14],&Uart_sand_byte[15]);				//yaw_pid

		// 数据解算
		joy[0] = C_rx_buf[1];//按键值
		joy[1] = C_rx_buf[2];//编码器值
		joy[2] = C_rx_buf[3];//编码器值 
		joy[3] = two_uint8_to_float_signed(C_rx_buf[8],C_rx_buf[9]);  									//RX摇杆值
		joy[4] = two_uint8_to_float_signed(C_rx_buf[10],C_rx_buf[11]);	
		//RY摇杆值
		joy_yaw_num = -two_uint8_to_float_signed(C_rx_buf[4],C_rx_buf[5]);								//lx摇杆值
		
		gimble_yaw = two_uint8_to_float_signed(Uart_recv_byte[1],Uart_recv_byte[2]);					//YAW电机编码器角度
		normal_yaw = normalizeAngleRad(gimble_yaw);
		gimbal_to_chassis_speed_compute(joy[3],joy[4],gimble_yaw - gimble_angal,&gimble_chassis[0],&gimble_chassis[1]);//小陀螺移动速度解算
		
		if((joy[0] & 0x01) != 0x01){
			OmniKinematics((int)gimble_chassis[0],(int)gimble_chassis[1],joy[1]*3);//小陀螺
		}else{
			OmniKinematics(joy[3],joy[4],yaw_output[1]);//云台跟随
		}	
//		OmniKinematics(0,0,50);
//		MecanumKinematics(joy[3],joy[4],0);
}
void uart_sand(void)
{
	HAL_UART_Transmit(&g_uart1_handle,Uart_sand_byte,sizeof(Uart_sand_byte),1000);
}
void data_print(void)
{
//		printf("%x,%d,%d,%d,%d,%d,%d\r\n",key_num,rx,ry,lx,ly,en1,en2);	
//		for(int i=0;i<20;i++){
//			printf("%2x,",C_rx_buf[i]);
//		}printf("\r\n");
//		for(int i=0;i<5;i++){
//			printf("%d,",joy[i]);
//		}printf("\r\n");
//		printf("%.2f,%.2f,%.2f\n",INS_angle_filtered[0],INS_angle_filtered[1],INS_angle_filtered[2]);	
		printf("%.2f,%.2f,%.2f\n",INS_angle[0],INS_angle[1],INS_angle[2]);
//		printf("%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\r\n",accel[0], accel[1], accel[2],gyro[0], gyro[1], gyro[2]);
//		printf("%.2f,%.2f,%.2f\n", accel[0], accel[1], accel[2]);
//		printf("%.2f\r\n",gimble_yaw);
//		printf("%.2f\r\n",yaw_output[1]);
	

}

/**
 * @brief 计算 4 个麦克纳姆轮的目标速度
 * @param Vx X 轴速度（m/s）
 * @param Vy Y 轴速度（m/s）
 * @param omega 旋转角速度（rad/s）
 */
void MecanumKinematics(int Vx, int Vy, int omega) 
{
    // 计算轮速
    wheel_speeds[0] = Vx - Vy - omega * (LX + LY);  // Wheel1
    wheel_speeds[1] = Vx + Vy + omega * (LX + LY);  // Wheel2
    wheel_speeds[2] = Vx + Vy - omega * (LX + LY);  // Wheel3
    wheel_speeds[3] = Vx - Vy + omega * (LX + LY);  // Wheel4

    // 限幅处理（防止超速）
    for (int i = 0; i < 4; i++) {
        if (wheel_speeds[i] > MAX_SPEED) {
            wheel_speeds[i] = MAX_SPEED;
        } else if (wheel_speeds[i] < -MAX_SPEED) {
            wheel_speeds[i] = -MAX_SPEED;
        }
    }
		MOTOR_PWM_UPDATE(data_kp*wheel_speeds[0],data_kp*wheel_speeds[1],data_kp*wheel_speeds[2],data_kp*wheel_speeds[3]);
}
/**
 * @brief 全向轮底盘速度解算（基于正方形布局，辊子径向安装）
 * @param Vx  X轴速度（m/s），正前方为正
 * @param Vy  Y轴速度（m/s），向左为正
 * @param omega  旋转角速度（rad/s），逆时针为正
 *
 * 注：原麦克纳姆轮函数已改为全向轮解算，请按注释重新接线！
 */
void OmniKinematics(int Vx, int Vy, int omega)
{
    wheel_speeds[0] =  - Vx + Vy - omega * (LX + LY);
    wheel_speeds[1] =    Vx + Vy - omega * (LX + LY);
    wheel_speeds[2] =    Vx - Vy - omega * (LX + LY);
    wheel_speeds[3] =  - Vx - Vy - omega * (LX + LY);

    for (int i = 0; i < 4; i++) {
        if (wheel_speeds[i] > MAX_SPEED) wheel_speeds[i] = MAX_SPEED;
        else if (wheel_speeds[i] < -MAX_SPEED) wheel_speeds[i] = -MAX_SPEED;
    }

    MOTOR_PWM_UPDATE(data_kp*wheel_speeds[0], data_kp*wheel_speeds[1], 
                     data_kp*wheel_speeds[2], data_kp*wheel_speeds[3]);
}
/**
 * @brief 云台坐标系速度 → 底盘坐标系速度（二维平面，仅考虑偏航角）
 * @param gimbal_vx  云台坐标系X轴速度（单位：m/s 或 mm/s，需统一）
 * @param gimbal_vy  云台坐标系Y轴速度（同上）
 * @param yaw_rad    云台相对底盘的偏航角差（单位：弧度，顺时针为正/逆时针为正需与硬件一致）
 * @param chassis_vx 输出：底盘坐标系X轴速度
 * @param chassis_vy 输出：底盘坐标系Y轴速度
 * @return true=计算成功，false=输入异常
 */
 void gimbal_to_chassis_speed_compute(float gimbal_vx, float gimbal_vy, 
                                     float yaw_rad,
                                     float *chassis_vx, float *chassis_vy) {

    float cos_yaw = cosf(yaw_rad);  
    float sin_yaw = sinf(yaw_rad);

    *chassis_vx = gimbal_vx * cos_yaw - gimbal_vy * sin_yaw;
    *chassis_vy = gimbal_vx * sin_yaw + gimbal_vy * cos_yaw;
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
    int16_t scaled = (int16_t)((high_byte << 8) | low_byte);
    
    // 缩小100倍
    return (float)scaled / 100.0f;
}

/**
 * 连续角度读取
 * @param 
 * @return: 
 */
float process_continuous_angle(uint8_t axis, float curr) 
{
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
float normalizeAngleRad(float angle)
{
    const float two_pi = 2 * PI;
    const float pi = PI;

    angle = fmodf(angle, two_pi);
    if (angle > pi)
        angle -= two_pi;
    else if (angle < -pi)
        angle += two_pi;

    return angle;
}
