#include "oled_app.h"

void System_State_Uart_Print(void);

void *navigator_;
unsigned char oled_clear_flag = 0;
unsigned char statr_flag = 0;

void Oled_Init(void)
{
  Uart_Printf(DEBUG_UART, "Oled_Init ......\r\n");
  OLED_Init();
  
  OLED_ShowPic(48, 0, 80, 4, LOGO);

  navigator_ = menu_builder(getMainItem());
}

void Oled_Task(void)
{
  menu_refresh_display(navigator_);
  
  char *ptrDisplayBuffer = menu_get_display_buffer(navigator_);
  
  if(statr_flag == 0)
  {
    oled_clear_flag = 0;
    statr_flag = 1;
    OLED_Clear();
  }
  
  /* 菜单系统运行 */
  for(unsigned char line = 0; line < MAX_DISPLAY_ITEM; line++)
  {
        OLED_ShowStr(0, line, ptrDisplayBuffer + (MAX_DISPLAY_CHAR * line), 8);
        unsigned char len = strlen(ptrDisplayBuffer + (MAX_DISPLAY_CHAR * line));
        for(unsigned char x = len; x < MAX_DISPLAY_CHAR; x++)
        {
            OLED_ShowChar(x * 8, line, ' ', 8);
        }
  }
  
  System_State_Uart_Print();
}

void System_State_Uart_Print(void)
{
  Uart_Printf(DEBUG_UART, "\r\n========== 系统状态 ==========\r\n");
  
  Uart_Printf(DEBUG_UART, "[RTC 实时时钟] 20%02d-%02d-%02d  %02d:%02d:%02d\r\n", RTC_Day.Year, RTC_Day.Month, RTC_Day.Date, RTC_Time.Hours, RTC_Time.Minutes, RTC_Time.Seconds);
  
  Uart_Printf(DEBUG_UART, "[PID 运行状态] %d\r\n", pid_running);
  
  Uart_Printf(DEBUG_UART, "[LED] %d-%d-%d-%d\r\n", led_buf[0], led_buf[1], led_buf[2], led_buf[3]);
  
  Uart_Printf(DEBUG_UART, "[灰度传感器] %d-%d-%d-%d-%d-%d-%d-%d\r\n",(gray_digtal>>0)&0x01,(gray_digtal>>1)&0x01,(gray_digtal>>2)&0x01,(gray_digtal>>3)&0x01,
                                                                   (gray_digtal>>4)&0x01,(gray_digtal>>5)&0x01,(gray_digtal>>6)&0x01,(gray_digtal>>7)&0x01);

  Uart_Printf(DEBUG_UART, "[速度环] left:%.2f, right:%.2f\r\n", pid_speed_left.target, pid_speed_right.target);

  Uart_Printf(DEBUG_UART, "[编码器] left:%.2fcm/s,right:%.2fcm/s\r\n", left_encoder.speed_cm_s, right_encoder.speed_cm_s);
  
  #if BNO08x_ON == 0
  Uart_Printf(DEBUG_UART, "[ICM20608 姿态传感器] %.2f, %.2f, %.2f\n", icm20608.Roll, icm20608.Pitch, icm20608.Yaw);
  #else
  Uart_Printf(DEBUG_UART, "[BNO08x 姿态传感器] %.2f, %.2f, %.2f\n", bno08x.Roll, bno08x.Pitch, bno08x.Yaw);
  #endif
  Uart_Printf(DEBUG_UART, "[温度] %.2f \r\n", Humiture.Temp);

  Uart_Printf(DEBUG_UART, "[湿度] %.2f\r\n", Humiture.RH);
  
  Uart_Printf(DEBUG_UART, "[当前页面] %s\r\n", menu_get_current_page_name(navigator_));
  
  Uart_Printf(DEBUG_UART, "[重启次数] %d\r\n", start_count);
  
  Uart_Printf(DEBUG_UART, "\r\n=============================\r\n");
}


