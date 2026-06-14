#ifndef __SEND_PRINTF_H__
#define __SEND_PRINTF_H__

#include "lz_hardware.h"

#define UART0_ID 0
void Usart_init( unsigned int BaudRate);
void SYN_FrameInfo(char Music, char HZdata[], char leng);
void su03t_play(uint8_t *msg, int len);

#endif