#include "Data.h"
#include <stdint.h>
#include <math.h>
#include "uart1.h"
#include <stdint.h>
#include <string.h>
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

uint8_t uart_recv[20] = {0};
uint8_t send_angal[20] = {0x55,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xff};
	
float zero_angal = 0.1f;	
float yaw_num = 0.0f;
float joy_num = 0.0f;
float yaw_pid_num = 0.0f;
	
//电机相关
	
float sensor_direction =1;//编码器方向
float pole_pairs =14;//电极对数
float voltage_power_supply =11.7;//供电电压
float Ualpha=0,Ubeta=0,Ua=0,Ub=0,Uc=0;
//float zero_electric_angle = 2.297887f;
float zero_electric_angle = 0;
//PID相关
PID PID_Pos;//位置PID
PID PID_Vel;//速度PID
PID PID_Cur;//电流PID

float position;//位置
float velocity;//速度
float current;//电流

//电流采集相关
uint16_t adc_result[4] = {0};
float gain_a,gain_b,gain_c;
float offset_ia,offset_ib,offset_ic;//采集的3路电流
float volts_to_amps_ratio;

float shunt_resistor = 0.01;//采样电阻值
float amp_gain  = 50;//运放增益
float current_a,current_b;//帕克变换的值
/*
	帧格式
	0x55 按键 enl enr lxh lxl lyh lyl
	rxh  rxl  ryh ryl yaw yaw pid pid
	0    0    0   0xff
*/
void data_change(void)
{ 
        uart1_poll();	
	
        float_to_two_uint8_signed(Get_Angel(),&send_angal[1],&send_angal[2]);
	
		joy_num = two_uint8_to_float_signed(uart_recv[6],uart_recv[7]);
        yaw_num = two_uint8_to_float_signed(uart_recv[12],uart_recv[13]);
		yaw_pid_num = two_uint8_to_float_signed(uart_recv[14],uart_recv[15]);
        
//		PositionCloseloop(0);
	
//		Position_VelocityCloseloop(0.65+(joy_num*0.4));
//		Position_VelocityCloseloop(0.2+yaw_pid_num);
		velocityOpenloop(3);
}
void data_print(void)
{
		
//		printf("%.2f,%.2f,%.2f\r\n",joy_num,yaw_num,yaw_pid_num);
		

//        for(uint16_t i = 0; i < 20; i++)
//        {
//            printf("%02X ", uart_recv[i]);
//        }printf("\r\n");
	
//		VelocityCloseloop(30);
//		velocityOpenloop(3);
//		Velocity_CurrentCloseloop(10);
//		Position_CurrentCloseloop(0);
//		
//		printf("%f\r\n",_electricalAngle());
//		printf("%.2f\r\n",Get_Angel());

		printf("%.2f\r\n",as5600_read_angal());
//		printf("%.2f\r\n",Get_Velocity());
//		
//		printf("%.2f,%.2f,%.2f,%.2f\r\n", (float)adc_result[0] / 4096 * 1.65,(float)adc_result[1] /
//					4096 * 1.65,(float)adc_result[2] / 4096 * 1.65,(float)adc_result[3] / 4096 * 1.65);

}
void data_init(void)
{


}
void float_to_two_uint8_signed(float num, uint8_t* high_byte, uint8_t* low_byte) {
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
float two_uint8_to_float_signed(uint8_t high_byte, uint8_t low_byte) {
    // 组合为int16_t（注意：高位需要符号扩展）
    int16_t scaled = (int16_t)((high_byte << 8) | low_byte);
    
    // 缩小100倍
    return (float)scaled / 100.0f;
}

//单变量低通滤波
float lowpass_filter_float(float new_val, float alpha)
{
    static float filtered = 0.0f;  // 注意：此写法只能用于单个变量滤波
    filtered = alpha * new_val + (1.0f - alpha) * filtered;
    return filtered;
}
//串口字符排序
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
