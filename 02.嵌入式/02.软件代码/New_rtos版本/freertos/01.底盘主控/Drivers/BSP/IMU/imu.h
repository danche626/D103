#ifndef __IMU_H
#define __IMU_H

#include "sys.h"

void AHRS_init(float quat[4], float accel[3], float mag[3]);    
void AHRS_update(float quat[4], float time, float gyro[3], float accel[3], float mag[3]);
void get_angle(float q[4], float *yaw, float *pitch, float *roll);void imu_updata(void);
void imu_updata(void);
void imu_init(void);
#endif
