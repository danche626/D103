#include "data.h"
#include "sys.h"
#include "delay.h"
#include "stdlib.h"
#include "math.h"
#include "usart.h"
#include "led.h"
#include "my_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc.h"
#include "encoder.h"
#include "oled.h"
#include "key.h"
#include "nrf24L01.h"

uint8_t key_num = {0};
uint8_t key_arr[10] = {1,1,1,1,1,1,1,1,1,1}; 
uint8_t key_prev_state[10] = {1,1,1,1,1,1,1,1,1,1};
char lx_buffer[20];
char ly_buffer[20];
char rx_buffer[20];
char ry_buffer[20];
char key_num_buffer[20];
char l_en[20];
char r_en[20];

uint16_t adc_value[4] = {0};
uint8_t Send_Out[32] = {0};
int16_t Joystick_value[4] = {0};
int16_t encoder_value[2] = {0};
float Joystick_Points_value[4];


/*
	帧格式
	0x55 按键 enl enr lxh lxl lyh lyl
	rxh  rxl  ryh ryl 0   0   0   0
	0    0    0   0xff
*/
void data_change(void)
{
	
		// 获取按键状态
		key_scan(); 
		
		// 获取编码器值
		encoder_value[0] = encoder_l();
		encoder_value[1] = encoder_r();	
	
		// 遥控器值-128 到 +128
		Joystick_value[0] = map0_4096To128_128WithMaskOpt(adc_value[2]); // lx
		Joystick_value[1] = map0_4096To128_128WithMaskOpt(adc_value[3]); // ly
		Joystick_value[2] = map0_4096To128_128WithMaskOpt(adc_value[1]); // rl
		Joystick_value[3] = map0_4096To128_128WithMaskOpt(adc_value[0]); // ry
		
		// 遥控器积分值
		Joystick_Points_value[0] = Joystick_Points_value[0] + Joystick_value[0] * Joystick_Points_kp;// lx
		Joystick_Points_value[1] = Joystick_Points_value[1] + Joystick_value[1] * Joystick_Points_kp;// ly
	
		Joystick_Points_value[1] = limit_float(Joystick_Points_value[1],-0.35f,0.8f);
//		Joystick_Points_value[0] = limit_float(Joystick_Points_value[0],-1.0f ,1.0f);
	

		floatToTwoSint8(Joystick_Points_value[0],&Send_Out[4],&Send_Out[5]);// lx
		floatToTwoSint8(Joystick_Points_value[1],&Send_Out[6],&Send_Out[7]);// ly		
		floatToTwoSint8(Joystick_value[2],&Send_Out[8],&Send_Out[9]);		// rl
		floatToTwoSint8(Joystick_value[3],&Send_Out[10],&Send_Out[11]);		// ry
		
		Send_Out[0]  = 0x55;
		Send_Out[1]  = key_num;
		Send_Out[2]  = encoder_value[0];
		Send_Out[3]  = encoder_value[1];
		Send_Out[19] = 0xff;
		
}

void data_print(void)
{
		for(int i=0;i<20;i++){
			printf("%2x,",Send_Out[i]);
		}printf("\r\n");
	
//        printf("%x,%d,%d\r\n",key_num,encoder_value[0],encoder_value[1]);
//		printf("%d,%d,%d,%d\r\n",adc_value[0],adc_value[1],adc_value[2],adc_value[3]);
//		printf("%.2f,%.2f\r\n",Joystick_Points_value[0],Joystick_Points_value[1]);
	
//		printf("%d,%d,%d,%d\r\n",Joystick_value[0],Joystick_value[1],Joystick_value[2],Joystick_value[3]);

//		HAL_UART_Transmit(&g_uart1_handle, Send_Out, sizeof(Send_Out), 1000);
//		printf("\r\n");
}
void oled_show_data(void)
{

//		oled_fill(0x00);

//		snprintf(lx_buffer, 8,"lx:%.2f",Joystick_Points_value[0]);
//		snprintf(ly_buffer, 8,"ly:%.2f",Joystick_Points_value[1]);
//		snprintf(rx_buffer, sizeof(rx_buffer),"ry:%+4d",Joystick_value[2]);
//		snprintf(ry_buffer, sizeof(ry_buffer),"rx:%+4d",Joystick_value[3]);
		
//		snprintf(l_en, sizeof(l_en),"len:%+4d",encoder_value[0]);
//		snprintf(r_en, sizeof(r_en),"ren:%+4d",encoder_value[1]);	
		
//		oled_show_string(0 , 0,l_en, 12);
//		oled_show_string(64, 0,r_en, 12);
		
//		oled_show_string(0,  2,lx_buffer, 12);
//		oled_show_string(64, 2,ry_buffer, 12);
//		oled_show_string(0,  4,ly_buffer, 12);
//		oled_show_string(64, 4,rx_buffer, 12);	
	
//		oled_show_keynum_bin(0, 6, key_num, 12);
}



// 优化版：将0-4096映射到-128~128，且屏蔽-10~+10区间
int16_t map0_4096To128_128WithMaskOpt(uint16_t raw_value) 
{
    // 计算映射值（简化公式，去掉冗余的-0）
    int32_t mapped = (int32_t)raw_value * 256 / 4096 - 128;
    
    // 范围限制 + 区间屏蔽（合并判断）
    mapped = (mapped > 128) ? 128 : (mapped < -128) ? -128 : mapped;
    mapped = (mapped >= -20 && mapped <= 20) ? 0 : mapped;
    
    return (int16_t)mapped;
}
/**
  * @brief  带符号浮点数转两个uint8_t（范围-327.68~327.67，两位小数）
  * @param  num: 输入浮点数（范围-327.68~327.67）
  * @param  high_byte: 高位字节输出
  * @param  low_byte: 低位字节输出
  * @note   精度0.01，有效范围-327.68~327.67
  */
void floatToTwoSint8(float num, uint8_t* high_byte, uint8_t* low_byte) 
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
float twoSint8ToFloat(uint8_t high_byte, uint8_t low_byte) 
{
    // 组合为int16_t（注意：高位需要符号扩展）
    int16_t scaled = (int16_t)((high_byte << 8) | low_byte);
    
    // 缩小100倍
    return (float)scaled / 100.0f;
}
// 全局通用 浮点限幅函数，带返回值，Keil 完美编译
float limit_float(float val, float min, float max)
{
    val = (val < min) ? min : val;
    val = (val > max) ? max : val;
    return val;
}
/**
 * @brief  将10位按键数组key_arr转换为8位key_num（映射前8个按键）
 * @param  key_arr  输入：10位按键状态数组（元素为0/1，0=按下，1=未按）
 * @retval key_num  输出：8位无符号整数，每一位对应key_arr前8个按键状态
 * @note   映射规则：key_arr[0]→key_num第7位，key_arr[1]→第6位...key_arr[7]→第0位
 */
uint8_t KeyArr_To_KeyNum(uint8_t key_arr[10])
{
    if(key_arr == NULL) return 0xFF; // 空指针保护，返回默认值（全1=无按键）
    
    uint8_t key_num = 0; // 初始化key_num为0
    
    // 遍历key_arr前8位，映射到key_num的8个二进制位
    for(uint8_t i = 0; i < 8; i++)
    {
        // 仅当key_arr[i]为0（按键按下）时，将key_num对应位清0；否则为1（未按）
        // 逻辑：key_num的第(7-i)位 = key_arr[i]（1=未按，0=按下）
        if(key_arr[i] == 1)
        {
            key_num |= (1 << (7 - i)); // 未按则对应位设1
        }
        // 按下则对应位保持0（无需操作，因key_num初始为0）
    }
    
    return key_num;
}
// 新增：将8位key_num转为"11111111"格式的纯二进制字符串
void KeyNum_To_BinStr(uint8_t key_num, char *bin_str)
{
    if(bin_str == NULL) return; // 空指针保护
    uint8_t i;
    // 逐位解析：从最高位（第7位）到最低位（第0位）
    for(i = 0; i < 8; i++)
    {
        // 按位与判断当前位是1还是0，转为字符'1'/'0'
        bin_str[i] = (key_num & (1 << (7 - i))) ? '1' : '0';
    }
    bin_str[8] = '\0'; // 字符串结束符，避免乱码
}

// 新增：在OLED上显示key_num的二进制值
void oled_show_keynum_bin(uint8_t x, uint8_t y, uint8_t key_num, uint8_t size)
{
    char bin_str[9] = {0}; // 存储8位二进制字符串（+结束符）
    KeyNum_To_BinStr(key_num, bin_str); // 转换为二进制字符串
    
    // 先显示标签，再显示二进制值（示例：标签"Key:" + 二进制值）
    oled_show_string(x, y, "Key:", size);          // 显示标签
    oled_show_string(x + size*2, y, bin_str, size); // 显示二进制值（偏移标签宽度）
}
