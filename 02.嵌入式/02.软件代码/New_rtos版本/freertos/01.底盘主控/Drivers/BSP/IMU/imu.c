#include "imu.h"
#include "math.h"
#include "MahonyAHRS.h"
#include "bmi088.h"
#include "bmi055.h"
#include "ist8310.h"
#include "usart.h"
#include "data.h"

void imu_init(void)
{
	AHRS_init(INS_quat,gyro,mag);
}
void imu_updata(void)
{
	BMI055_update(&accel[0],&accel[1],&accel[2],&gyro[0],&gyro[1],&gyro[2],&temp[0]);
	//ist8310_updata(&mag[0],&mag[1],&mag[2],&temp[1]);
	
	// 2. 单位转换：Mahony 必须使用 rad/s
    float gyro_rad[3];
    gyro_rad[0] = gyro[0] * (PI / 180.0f);
    gyro_rad[1] = gyro[1] * (PI / 180.0f);
    gyro_rad[2] = gyro[2] * (PI / 180.0f);
	
	AHRS_update(INS_quat,0.001f,gyro_rad,accel,mag);
	
	get_angle(INS_quat,&INS_angle[0],&INS_angle[1],&INS_angle[2]);
	
	INS_continuous_angle[0] = process_continuous_angle(0, INS_angle[0]); 
    INS_continuous_angle[1] = process_continuous_angle(1, INS_angle[1]); 
    INS_continuous_angle[2] = process_continuous_angle(2, INS_angle[2]); 
	
	// 【必选：对连续角度滤波（给PID用，解决Kd振动）】
    INS_angle_filtered[0] = low_pass_filter(INS_continuous_angle[0], &angle_lpf[0]);
    INS_angle_filtered[1] = low_pass_filter(INS_continuous_angle[1], &angle_lpf[1]);
    INS_angle_filtered[2] = low_pass_filter(INS_continuous_angle[2], &angle_lpf[2]);
	
}
void AHRS_init(float quat[4], float accel[3], float mag[3])
{
    quat[0] = 1.0f;
    quat[1] = 0.0f;
    quat[2] = 0.0f;
    quat[3] = 0.0f;
}

void AHRS_update(float quat[4], float time, float gyro[3], float accel[3], float mag[3])
{
    //MahonyAHRSupdate(quat, gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2], mag[0], mag[1], mag[2]);
	MahonyAHRSupdateIMU(quat, gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2]);
}

void get_angle(float q[4], float *yaw, float *pitch, float *roll)
{
    *yaw = atan2f(2.0f*(q[0]*q[3]+q[1]*q[2]), 2.0f*(q[0]*q[0]+q[1]*q[1])-1.0f);
    *pitch = asinf(-2.0f*(q[1]*q[3]-q[0]*q[2]));
    *roll = atan2f(2.0f*(q[0]*q[1]+q[2]*q[3]),2.0f*(q[0]*q[0]+q[3]*q[3])-1.0f);
}

