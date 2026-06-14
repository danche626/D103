#ifndef __ROBOT_SG90_H__
#define __ROBOT_SG90_H__

#define Window_PIN     GPIO1_PD0
#define Car_PIN        GPIO0_PB4
#define Door_PIN       GPIO0_PB5

typedef int (*printf_fn)(const char *fmt, ...);

void init_sg90(void);
void engine_turn_left(GpioID id);
void engine_turn_right(GpioID id);
void regress_90(GpioID id);
void set_angle(GpioID id, unsigned int duty);
void set_angle_ms(GpioID id, unsigned int duty, int ms);
void door_open(void);
void door_close(void);
void window_open(void);
void window_close(void);
void car_door_open(void);
void car_door_close(void);


/*

180°
0       20ms      0.5ms
45      20ms      1ms
90      20ms      1.5ms
135     20ms      2ms
180     20ms      2.5ms

360°
正转 周期20ms      脉冲高电平时间 0.5~1.5ms    快到慢
反转 周期20ms      脉冲高电平时间 1.5~2.5ms    慢到快
暂停 周期20ms      脉冲高电平时间 1.5ms    
转速 周期20ms      脉冲高电平时间 2.5ms    


*/

#endif