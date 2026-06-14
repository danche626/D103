#ifndef LOWPASS_H
#define LOWPASS_H

#include "FOC.h"

typedef struct 
{
	float Tf; 
	float y_prev; 
	unsigned long timestamp_prev;  
} LowPassFilter;

// 高通滤波器结构体（与低通保持一致风格）
typedef struct {
    float Tf;                // 滤波器时间常数
    unsigned long timestamp_prev; // 上一次时间戳
    float x_prev;            // 上一次输入值
    float y_prev;            // 上一次输出值
} HighPassFilter;

extern LowPassFilter  LPF_current,LPF_velocity;


void LOWPass_Init(void);
float LowPassFilter_operator(LowPassFilter *Lfi,float x);


#endif


