#ifndef FOC_H
#define FOC_H

#include "AS5600.h"
#include "Data.h"

#define _constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

extern float sensor_direction;
extern float pole_pairs;
extern float voltage_power_supply;
extern float zero_electric_angle;
extern float Ualpha,Ubeta,Ua,Ub,Uc;

void setPhaseVoltage_new(float Uq, float Ud, float angle_el);
void setPhaseVoltage(float Uq, float Ud, float angle_el);
float _normalizeAngle(float angle);
float _electricalAngle(void);



#endif 

