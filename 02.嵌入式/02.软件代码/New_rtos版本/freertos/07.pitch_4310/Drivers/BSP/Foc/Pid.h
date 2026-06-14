#ifndef PID_H
#define PID_H

#include "sys.h"
#define _constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

typedef struct{
    float P;
    float I;
    float D;
    float output;
    float limit;
    float output_ramp;
    
    // 原有变量
    float error_prev;
    float output_prev;
    float integral_prev;
    unsigned long timestamp_prev;
    

} PID;

float PID_operator(PID *pid, float error);

#endif 
