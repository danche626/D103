#include "FOC.h"
#include "pwm.h"
#include "math.h"
#include "uart1.h"
#include "Data.h"

// 归一化角度到 [0,2PI]
float _normalizeAngle(float angle){
  float a = fmod(angle, 2*PI);   //取余运算可以用于归一化，列出特殊值例子算便知
  return a >= 0 ? a : (a + 2*PI);  
}

//求解电角度
float _electricalAngle(void){
  	return  _normalizeAngle((float)(sensor_direction * pole_pairs) * Get_Angel_Notrack()-zero_electric_angle);
}


void setPhaseVoltage(float Uq, float Ud, float angle_el)
{
	float Uout;
	uint32_t sector;
	float T0,T1,T2;
	float Ta,Tb,Tc;
	
	if(Ud)
	{
		Uout = _sqrt(Ud*Ud + Uq*Uq) / voltage_power_supply;
		angle_el = _normalizeAngle(angle_el + atan2(Uq, Ud));
	}
	else
	{
		Uout = Uq / voltage_power_supply;
		angle_el = _normalizeAngle(angle_el + _PI_2);
	}
	if(Uout> 0.577)Uout= 0.577;
	if(Uout<-0.577)Uout=-0.577;
	
	sector = (angle_el / _PI_3) + 1;
	T1 = sqrt(3)*sin(sector*_PI_3 - angle_el) * Uout;
	T2 = sqrt(3)*sin(angle_el - (sector-1.0)*_PI_3) * Uout;
	T0 = 1 - T1 - T2;
	
	//扇区计算
	switch(sector)
	{
		case 1:
			Ta = T1 + T2 + T0/2;
			Tb = T2 + T0/2;
			Tc = T0/2;
			break;
		case 2:
			Ta = T1 +  T0/2;
			Tb = T1 + T2 + T0/2;
			Tc = T0/2;
			break;
		case 3:
			Ta = T0/2;
			Tb = T1 + T2 + T0/2;
			Tc = T2 + T0/2;
			break;
		case 4:
			Ta = T0/2;
			Tb = T1+ T0/2;
			Tc = T1 + T2 + T0/2;
			break;
		case 5:
			Ta = T2 + T0/2;
			Tb = T0/2;
			Tc = T1 + T2 + T0/2;
			break;
		case 6:
			Ta = T1 + T2 + T0/2;
			Tb = T0/2;
			Tc = T1 + T0/2;
			break;
		default:  
			Ta = 0;
			Tb = 0;
			Tc = 0;
	}

//	pwm_compare_set(Ta*1440,Tb*1440,Tc*1440);
	
	motor1_pwm_set(Ta*1440,Tb*1440,Tc*1440);
//	motor2_pwm_set(Ta*1440,Tb*1440,Tc*1440);
}

