#ifndef __ADC_H__
#define __ADC_H__

#include "sys.h"

void adc_dma_init(uint32_t *mar);

float _readADCVoltage(uint16_t ch);
float cal_Iq_Id(float current_a,float current_b,float angle_el);	
void Current_calibrateOffsets(void);
void CurrSense_init(void);
void CurrSense_getPhaseCurrents(void);
float Get_Current(void);

#endif
