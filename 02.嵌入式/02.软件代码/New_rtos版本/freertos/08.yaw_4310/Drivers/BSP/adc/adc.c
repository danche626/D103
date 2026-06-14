#include "adc.h"
#include "Data.h"
#include "Lowpass.h"
#include "math.h"
#include "delay.h"
#include "uart1.h"

ADC_HandleTypeDef adc_handle = {0};
DMA_HandleTypeDef dma_handle = {0};

void adc_config(void)
{
    adc_handle.Instance = ADC1;
    adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc_handle.Init.ScanConvMode = ADC_SCAN_ENABLE;
    adc_handle.Init.ContinuousConvMode = ENABLE;
    adc_handle.Init.NbrOfConversion = 4;
    adc_handle.Init.DiscontinuousConvMode = DISABLE;
    adc_handle.Init.NbrOfDiscConversion = 0;
    adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    HAL_ADC_Init(&adc_handle);
    
    HAL_ADCEx_Calibration_Start(&adc_handle);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        RCC_PeriphCLKInitTypeDef adc_clk_init = {0};
        GPIO_InitTypeDef gpio_init_struct = {0};
        
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        
        gpio_init_struct.Pin =  GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);
        
        adc_clk_init.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        adc_clk_init.AdcClockSelection = RCC_ADCPCLK2_DIV6;
        HAL_RCCEx_PeriphCLKConfig(&adc_clk_init);
    }
}

void dma_config(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();
    dma_handle.Instance = DMA1_Channel1;
    dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    
    //内存相关配置
    dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dma_handle.Init.MemInc = DMA_MINC_ENABLE;
    
    //外设相关配置
    dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;
    
    dma_handle.Init.Priority = DMA_PRIORITY_MEDIUM;
    dma_handle.Init.Mode = DMA_CIRCULAR;
    HAL_DMA_Init(&dma_handle);
    
    __HAL_LINKDMA(&adc_handle, DMA_Handle, dma_handle);
}

void adc_channel_config(ADC_HandleTypeDef* hadc, uint32_t ch, uint32_t rank, uint32_t stime)
{
    ADC_ChannelConfTypeDef adc_ch_config = {0};
    
    adc_ch_config.Channel = ch;
    adc_ch_config.Rank = rank;
    adc_ch_config.SamplingTime = stime;
    HAL_ADC_ConfigChannel(hadc, &adc_ch_config);
}

void adc_dma_init(uint32_t *mar)
{
    adc_config();
    adc_channel_config(&adc_handle, ADC_CHANNEL_0, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);
    adc_channel_config(&adc_handle, ADC_CHANNEL_1, ADC_REGULAR_RANK_2, ADC_SAMPLETIME_239CYCLES_5);
    adc_channel_config(&adc_handle, ADC_CHANNEL_4, ADC_REGULAR_RANK_3, ADC_SAMPLETIME_239CYCLES_5);
    adc_channel_config(&adc_handle, ADC_CHANNEL_5, ADC_REGULAR_RANK_4, ADC_SAMPLETIME_239CYCLES_5);
    dma_config();
    
    HAL_ADC_Start_DMA(&adc_handle, mar, 4);
}

float _readADCVoltage(uint16_t ch)
{
  return (float)ch*1.65/4096;
}

//通过Ia,Ib,Ic计算Iq,Id(目前仅输出Iq)
float cal_Iq_Id(float current_a,float current_b,float angle_el)
{
	float sign=1;
  	float I_alpha=current_a;
  	float I_beta = _1_SQRT3 * current_a + _2_SQRT3 * current_b;

	if(angle_el) sign = (I_beta * cos(angle_el) - I_alpha*sin(angle_el)) > 0 ? 1 : -1;  

	return sign*sqrt(I_alpha*I_alpha + I_beta*I_beta);
}

void Current_calibrateOffsets(void)
{
	offset_ia=0;
	offset_ib=0;

	for(int i=0; i<1000; i++)
	{
		offset_ia += _readADCVoltage(adc_result[0]);
		offset_ib += _readADCVoltage(adc_result[1]);
		delay_ms(1);
	}

	offset_ia = offset_ia/1000;
	offset_ib = offset_ib/1000;

}

void CurrSense_init(void)
{
	volts_to_amps_ratio = 1.0f /shunt_resistor / amp_gain; // volts to amps
    
    gain_a = volts_to_amps_ratio*1;
    gain_b = volts_to_amps_ratio*-1;
    gain_c = volts_to_amps_ratio;

	adc_dma_init((uint32_t  *)&adc_result);
	
	Current_calibrateOffsets();
}

void CurrSense_getPhaseCurrents(void)
{

    current_a = (_readADCVoltage(adc_result[0]) - offset_ia)*gain_a;
    current_b = (_readADCVoltage(adc_result[1]) - offset_ib)*gain_b;
	
//	float current_c = -current_a - current_b;
//	
//	printf("%.2f,%.2f,%.2f\r\n",current_a,current_b,current_c);
}

float Get_Current(void)
{
	CurrSense_getPhaseCurrents();
	float Iq_ori=cal_Iq_Id(current_a,current_b,_electricalAngle());
  	float Iq_flit=LowPassFilter_operator(&LPF_current,Iq_ori);
  	return Iq_flit;  
}
