#ifndef __BOARD_H
#define __BOARD_H

#include "stm32f10x.h"

/**********************************************************
***	Emm_V5.0²½½ø±Õ»·¿ØÖÆÀý³Ì
***	±àÐ´×÷Õß£ºZHANGDATOU
***	¼¼ÊõÖ§³Ö£ºÕÅ´óÍ·±Õ»·ËÅ·þ
***	ÌÔ±¦µêÆÌ£ºhttps://zhangdatou.taobao.com
***	CSDN²©¿Í£ºhttp s://blog.csdn.net/zhangdatou666
***	qq½»Á÷Èº£º262438510
**********************************************************/

void nvic_init(void);
void clock_init(void);
void usart_init(void);
void laser_gpio_init(void);
void laser_on(void);
void laser_off(void);
void board_init(void);

#endif
