#include "board.h"
#include "delay.h"
#include "usart.h"
#include "Emm_V5.h"
#include "math.h"
#include "stdlib.h"
#include "pid.h"

extern PIDTypdDef M1PID;
extern PIDTypdDef M2PID;

#define M1_PWMMAX 1000
#define M2_PWMMAX 1000
#define TRACK_CENTER_X 150.0f
#define TRACK_CENTER_Y 88.0f
#define LOCK_TOL_X 5
#define LOCK_TOL_Y 5
#define LOCK_COUNT_THRESHOLD 5
#define LOST_SENTINEL_X 300
#define LOST_SENTINEL_Y 300

int PWM1 = 672;
int PWM2 = 615;
int cx = LOST_SENTINEL_X;
int cy = LOST_SENTINEL_Y;

unsigned char CW1, CW2;
unsigned char len;
unsigned char target_valid = 0;
unsigned char laser_enabled = 0;
unsigned char lock_counter = 0;

static void laser_update_state(void)
{
	if(!target_valid)
	{
		lock_counter = 0;
		laser_enabled = 0;
		laser_off();
		return;
	}

	if((abs(cx - (int)TRACK_CENTER_X) <= LOCK_TOL_X) && (abs(cy - (int)TRACK_CENTER_Y) <= LOCK_TOL_Y))
	{
		if(lock_counter < 255)
		{
			lock_counter++;
		}
		if(lock_counter >= LOCK_COUNT_THRESHOLD)
		{
			laser_enabled = 1;
			laser_on();
		}
	}
	else
	{
		lock_counter = 0;
		laser_enabled = 0;
		laser_off();
	}
}

void kz_hs(int32_t M1pwm1, int32_t M2pwm2)
{
	if(M1pwm1 > 0)
	{
		CW1 = 1;
	}
	else
	{
		CW1 = 0;
	}

	if(M2pwm2 > 0)
	{
		CW2 = 0;
	}
	else
	{
		CW2 = 1;
	}

	M1pwm1 = abs(M1pwm1);
	M2pwm2 = abs(M2pwm2);

	if(target_valid)
	{
		Emm_V5_Pos_Control(1, CW1, 50, 0, M1pwm1, 0, 0);
		delay_ms(1);
		Emm_V5_Pos_Control(2, CW2, 50, 0, M2pwm2, 0, 0);
		delay_ms(70);
	}
}

void uart_update_target(void)
{
	if(USART_RX_STA & 0x8000)
	{
		len = USART_RX_STA & 0x3f;
		if(len == 7)
		{
			cx = (USART_RX_BUF[0] - '0') * 100 + (USART_RX_BUF[1] - '0') * 10 + (USART_RX_BUF[2] - '0');
			cy = (USART_RX_BUF[4] - '0') * 100 + (USART_RX_BUF[5] - '0') * 10 + (USART_RX_BUF[6] - '0');
			if((cx == LOST_SENTINEL_X) && (cy == LOST_SENTINEL_Y))
			{
				target_valid = 0;
			}
			else
			{
				target_valid = 1;
			}
		}
		USART_RX_STA = 0;
	}
}

void MODE1(void)
{
	PID_M1_SetPoint(TRACK_CENTER_X);
	PID_M1_SetKp(7);
	PID_M1_SetKi(0);
	PID_M1_SetKd(0.7f);

	PID_M2_SetPoint(TRACK_CENTER_Y);
	PID_M2_SetKp(3);
	PID_M2_SetKi(0);
	PID_M2_SetKd(0.3f);

	if(!target_valid)
	{
		laser_update_state();
		return;
	}

	PWM1 = PID_M1_PosLocCalc((float)cx);
	PWM2 = PID_M2_PosLocCalc((float)cy);

	if(PWM1 > M1_PWMMAX)
	{
		PWM1 = M1_PWMMAX;
	}
	if(PWM1 < -M1_PWMMAX)
	{
		PWM1 = -M1_PWMMAX;
	}
	if(PWM2 > M2_PWMMAX)
	{
		PWM2 = M2_PWMMAX;
	}
	if(PWM2 < -M2_PWMMAX)
	{
		PWM2 = -M2_PWMMAX;
	}

	kz_hs(PWM1, PWM2);
	laser_update_state();
}

int main(void)
{
	board_init();
	PID_M1_Init();
	PID_M2_Init();
	delay_ms(2000);
	laser_off();

	while(1)
	{
		uart_update_target();
		MODE1();
	}
}
