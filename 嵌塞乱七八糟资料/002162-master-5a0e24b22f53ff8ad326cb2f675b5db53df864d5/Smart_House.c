#include "smart_house.h"


static fifo_uart_t m_uart_fifo =
{
    .head.type                 = E_FIFO_TYPE_UART,
    .head.buffer_size          = sizeof(uart_rev_t),
};

static fifo_sound_t m_sound_fifo =
{
    .head.type                 = E_FIFO_TYPE_SOUND,
    .head.buffer_size          = sizeof(sound_play_t),
};


uint32_t       g_tcp_queue;
uint32_t       g_netrev_queue;
uint32_t       g_wifi_queue;
uint32_t       g_lcd_queue;

UINT32         g_i2c_muxlock;
lcd_param_t    lcd                = {.dir = 0, .f_color = LCD_LBBLUE, .b_color = LCD_WHITE, .f_size = 32, .start_line = IMAGE_H_SIEZ};
delay_ctrl_t   dctrl              = {.door = DELAY_TIME_MS, .no_key = DELAY_TIME_MS*2, .voice_window = DELAY_TIME_MS, .voice_car = TIM_MS_COUNT(1500)};
Sensor_data_t  Sensor_data        = {.fire_flag = 1, .luminance = 10000};           //传感器数据
Control_data_t Control_data       = {0};

static smart_house_t m_sh =
{
    .sound_str = 
    {
        [E_SOUND_TYPE_OPEN_DOOR]         = "门已打开",                            
        [E_SOUND_TYPE_CLOSE_DOOR]        = "门已关闭",           
        [E_SOUND_TYPE_OPEN_WINDOW]       = "窗户已打开",          
        [E_SOUND_TYPE_CLOSE_WINDOW]      = "窗户已关闭",           
        [E_SOUND_TYPE_OPEN_GARAGE_DOOR]  = "车库门已打开",             
        [E_SOUND_TYPE_CLOSE_GARAGE_DOOR] = "车库门已关闭",              
        [E_SOUND_TYPE_OPEN_ALL]          = "门窗都已打开",     
        [E_SOUND_TYPE_CLOSE_ALL]         = "门窗都已关闭",      
        [E_SOUND_TYPE_PSWD_ERR]          = "密码错误",        
        [E_SOUND_TYPE_PSWD_NOP]          = "密码不足四位",     
        [E_SOUND_TYPE_ALARM_FIRE]        = "火焰报警",          
        [E_SOUND_TYPE_ALARM_SMOKE]       = "烟雾报警",           
        [E_SOUND_TYPE_RAIN_CLOSE_WINDOW] = "雨滴检测关窗户",           
        [E_SOUND_TYPE_OPEN_LAMP]         = "灯已打开",     
        [E_SOUND_TYPE_CLOSE_LAMP]        = "灯已关闭",      
        [E_SOUND_TYPE_RINGTONE]          = "门铃声",          
        [E_SOUND_TYPE_ALARM]             = "报警声",         
        [E_SOUND_TYPE_WELCOME]           = "欢迎使用",       
        [E_SOUND_TYPE_UPVAL]             = "更新数据",
        [E_SOUND_TYPE_RESET]             = "复位语音模块",
        [E_SOUND_TYPE_SKIP_HEAT]         = "跳过预热",
        [E_SOUND_TYPE_PPM_CALIB]         = "烟雾校准",
        [E_SOUND_TYPE_PPMUP]             = "增加烟雾阈值",
        [E_SOUND_TYPE_PPMDOWN]           = "减少烟雾阈值",
        [E_SOUND_TYPE_LIGHTUP]           = "增加亮度阈值",
        [E_SOUND_TYPE_LIGHTDOWN]         = "减少亮度阈值",
    },
    .swtmr = 
    {
        .cnt                    = 0,
        .args                   = &m_sh,

        .getid_fn               = swtmr_getid,
        .start_fn               = swtmr_start,
        .stop_fn                = swtmr_stop,
        .init_fn                = swtmr_init,
    },

    .input_count                = 0,
    .password                   = 0,
    .key_val                    = {'-', '-', '-', '-'},
    
    .door.onoff                 = 0,
    .cardoor.onoff              = 0,
    .windows.onoff              = 0,

    .report_flag                = 0,
    .smoke_alarm_threshold      = SMOKE_VALUE,
    .brightness_threshold       = LUMINACE_DARK_VALUE,

    .lcd                        = &lcd,
    .dctrl                      = &dctrl,
    .sensor_data                = &Sensor_data,
    .control_data               = &Control_data,
};

static void wait_wdt_reboot(void)
{
    int wdt_type = 0; //立刻复位

    HAL_WDT_Init(HAL_CRU_ClkGetFreq(PCLK_MCU_BUS), WDT0);
    RequestIrq(WDT0_IRQn, NULL, WDT0); 

    HAL_CRU_WdtGlbRstEnable(wdt_type);
    HAL_WDT_SetTimeout(1);  
    printf(LOG_SH_TAG_INFO"reboot..........\r\n", __LINE__);
    LOS_Msleep(10);

    HAL_WDT_Start(wdt_type);  
    LOS_IntLock();

    while(1)
    {
        LOS_Msleep(10);
    }
}

// 设置烟雾报警阈值
int set_smoke_alarm_threshold(uint32_t v)
{
    smart_house_t  *sh  = &m_sh;

    sh->smoke_alarm_threshold = v;
    printf(LOG_SH_TAG_INFO"smoke_alarm_threshold:%d\n", __LINE__, v);
    return 0;
}

// 设置亮度阈值
int set_brightness_threshold(uint32_t v)
{
    smart_house_t  *sh  = &m_sh;

    sh->brightness_threshold = v;
    return 0;
}

// 增加烟雾阈值
int up_smoke_alarm_threshold(uint32_t v)
{
    smart_house_t  *sh  = &m_sh;

    return set_smoke_alarm_threshold(sh->smoke_alarm_threshold + v);
}

// 减少烟雾阈值
int down_smoke_alarm_threshold(uint32_t v)
{
    smart_house_t  *sh  = &m_sh;

    if(sh->smoke_alarm_threshold > v) v = sh->smoke_alarm_threshold - v;
    else v = SMOKE_SET_VALUE_MIN;
    v = v < SMOKE_SET_VALUE_MIN?SMOKE_SET_VALUE_MIN:v;
    return set_smoke_alarm_threshold(v);
}

// 增加亮度阈值
int up_brightness_threshold(uint32_t v)
{
    smart_house_t  *sh  = &m_sh;

    return set_brightness_threshold(sh->brightness_threshold + v);
}

// 减少亮度阈值
int down_brightness_threshold(uint32_t v)
{
    smart_house_t  *sh  = &m_sh;

    if(sh->brightness_threshold > v) v = sh->brightness_threshold - v;
    else v = LIGHT_SET_VALUE_MIN;
    v = v < LIGHT_SET_VALUE_MIN?LIGHT_SET_VALUE_MIN:v;
    return set_brightness_threshold(v);
}

// 消息发送到网络
static void smart_house_msg_send_to_net(void *msg, uint16_t cmd)
{
    smart_house_t  *sh  = &m_sh;
    dev_msg_t      *dev = (dev_msg_t*)msg;

    memcpy(dev->head.num, sh->devnum, sizeof(uint8_devnum));
    dev->head.type                 = Swap16(E_DEV_SMART_HOUSE);
    dev->smart_house.head.cmd_type = Swap16(cmd);

    // printf(LOG_SH_TAG_NET_DATA"cmd:%d, len:%d\n", __LINE__, (int)cmd, (int)Swap16(dev->head.len));

    // int        len = get_smart_house_msg_len(cmd);
    // dump_hex(printf, __FUNCTION__, (uint8_t*)&dev->smart_house, len);

    if(0 != LOS_QueueWriteCopy(g_tcp_queue, &dev->smart_house, sizeof(app_msg_data_t), QUEUE_DEFAULT_WRITE_TIME_MS))
    {
        WRITE_QUEUE_ERR_LOG(g_tcp_queue);
    }               
}

// 查找设备响应
static void smart_house_msg_send_to_net_search_dev(uint8_t response)
{
    dev_msg_t dev = {0};

    dev.smart_house.search_dev.response = response;
    smart_house_msg_send_to_net(&dev, E_SMART_HOUSE_CMD_SEARCH_DEV);
}

// 设备状态上报
static void smart_house_msg_send_to_net_report_state(uint8_t response, 
                                                   float   temperature, 
                                                   float   humidity, 
                                                   float   luminance,
                                                   uint8_t rain,
                                                   uint8_t ppm,
                                                   uint8_t fire)
{
    dev_msg_t dev = {0};
    uint16_t t, h, l;

    t = temperature*100;
    h = humidity*100;
    l = luminance>65535?65535:luminance;

    dev.smart_house.report_state.head.response  = response;
    dev.smart_house.report_state.temperature    = Swap16(t);
    dev.smart_house.report_state.humidity       = Swap16(h);
    dev.smart_house.report_state.luminance      = Swap16(l);
    dev.smart_house.report_state.rain           = rain;
    dev.smart_house.report_state.ppm            = ppm;
    dev.smart_house.report_state.fire           = fire;
    smart_house_msg_send_to_net(&dev, E_SMART_HOUSE_CMD_REPORT_STATE);
}

// 设备开关
static void smart_house_msg_send_to_net_onoff(uint8_t response, uint8_t onoff, smart_house_cmd_type_e cmd)
{
    dev_msg_t dev = {0};

                                                                        
    dev.smart_house.on_off.head.response  = response;
    dev.smart_house.on_off.onoff          = onoff;
    smart_house_msg_send_to_net(&dev, cmd);
}

// 解析并发送设备响应
static void net_data_response(dev_msg_t dev)
{
    smart_house_t *sh = &m_sh;
    delay_ctrl_t  *dc = sh->dctrl;
    Sensor_data_t *sd = sh->sensor_data;

    if(dev.smart_house.head.cmd_type != E_SMART_HOUSE_CMD_SEARCH_DEV && memcmp(dev.head.num, sh->devnum, DEVNUM_LEN) != 0 &&
    memcmp(dev.head.num, COMMON_DEV, DEVNUM_LEN) != 0)
    {
        uint8_t *devnum = dev.head.num;
        printf(LOG_SH_TAG_NET_DATA"other dev msg:%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x=============\n\n", __LINE__,
                devnum[0], devnum[1], devnum[2], devnum[3], devnum[4],  devnum[5],
                devnum[6], devnum[7], devnum[8], devnum[9], devnum[10], devnum[11]
        );
        return;
    }

    if(dev.head.type != E_DEV_SMART_HOUSE && dev.smart_house.head.cmd_type != E_SMART_HOUSE_CMD_RESET)
    {
        printf(LOG_SH_TAG_NET_DATA"dev type different:%04x=============\n\n", __LINE__, dev.head.type);
        return;
    }

    switch(dev.smart_house.head.cmd_type)
    {
        case E_SMART_HOUSE_CMD_SEARCH_DEV:
            printf(LOG_SH_TAG_NET_DATA"search dev response\n", __LINE__);
            smart_house_msg_send_to_net_search_dev(SMART_HOUSE_RESPONSE_SUCCESS);
            break;
        case E_SMART_HOUSE_CMD_REPORT_STATE:
            printf(LOG_SH_TAG_NET_DATA"report state\n", __LINE__);
            smart_house_msg_send_to_net_report_state(SMART_HOUSE_RESPONSE_SUCCESS, 
                sd->temperature, sd->humidity, sd->luminance, 
                sd->rain == 0?1:0, 
                (sd->ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0))?1:0, 
                sd->fire_flag == 0?1:0);
            break;
        case E_SMART_HOUSE_CMD_OPEN_DOOR:
            if(dev.smart_house.on_off.onoff == SMART_HOUSE_OPEN)
            {
                SU03T_Ctrl(E_SOUND_TYPE_OPEN_DOOR);
                Sound_play(E_SOUND_TYPE_OPEN_DOOR, 2000, "门已打开");
            }
            else
            {
                SU03T_Ctrl(E_SOUND_TYPE_CLOSE_DOOR);
                Sound_play(E_SOUND_TYPE_CLOSE_DOOR, 1500, "门已关闭");
            }
            printf(LOG_SH_TAG_NET_DATA"door:%s\n", __LINE__, dev.smart_house.on_off.onoff == SMART_HOUSE_OPEN ? "打开":"关闭");
            smart_house_msg_send_to_net_onoff(SMART_HOUSE_RESPONSE_SUCCESS, dev.smart_house.on_off.onoff, dev.smart_house.head.cmd_type);
            break;
        case E_SMART_HOUSE_CMD_OPEN_CAR_DOOR:
            if(dev.smart_house.on_off.onoff == SMART_HOUSE_OPEN)
            {
                SU03T_Ctrl(E_SOUND_TYPE_OPEN_GARAGE_DOOR);
                Sound_play(E_SOUND_TYPE_OPEN_GARAGE_DOOR, 2500, "车库已打开");
            }
            else
            {
                SU03T_Ctrl(E_SOUND_TYPE_CLOSE_GARAGE_DOOR);
            }
            printf(LOG_SH_TAG_NET_DATA"car door:%s\n", __LINE__, dev.smart_house.on_off.onoff == SMART_HOUSE_OPEN ? "打开":"关闭");
            smart_house_msg_send_to_net_onoff(SMART_HOUSE_RESPONSE_SUCCESS, dev.smart_house.on_off.onoff, dev.smart_house.head.cmd_type);
            break;
        case E_SMART_HOUSE_CMD_OPEN_WINDOWS:
            if(dev.smart_house.on_off.onoff == SMART_HOUSE_OPEN)
            {
                SU03T_Ctrl(E_SOUND_TYPE_OPEN_WINDOW);
                Sound_play(E_SOUND_TYPE_OPEN_WINDOW, 2500, "窗户已打开");
            }
            else
            {
                SU03T_Ctrl(E_SOUND_TYPE_CLOSE_WINDOW);
                Sound_play(E_SOUND_TYPE_CLOSE_WINDOW, 2500, "窗户已关闭");
            }
            printf(LOG_SH_TAG_NET_DATA"windows:%s\n", __LINE__, dev.smart_house.on_off.onoff == SMART_HOUSE_OPEN ? "打开":"关闭");
            smart_house_msg_send_to_net_onoff(SMART_HOUSE_RESPONSE_SUCCESS, dev.smart_house.on_off.onoff, dev.smart_house.head.cmd_type);
            break;
        case E_SMART_HOUSE_CMD_OPEN_LAMP:
            if(dev.smart_house.on_off.onoff == SMART_HOUSE_OPEN)
            {
                SU03T_Ctrl(E_SOUND_TYPE_OPEN_LAMP);
                Sound_play(E_SOUND_TYPE_OPEN_LAMP, 2500, "灯已打开");
            }
            else
            {
                SU03T_Ctrl(E_SOUND_TYPE_CLOSE_LAMP);
                Sound_play(E_SOUND_TYPE_CLOSE_LAMP, 2500, "灯已关闭");
            }
            printf(LOG_SH_TAG_NET_DATA"lamp:%s\n", __LINE__, dev.smart_house.on_off.onoff == SMART_HOUSE_OPEN ? "打开":"关闭");
            smart_house_msg_send_to_net_onoff(SMART_HOUSE_RESPONSE_SUCCESS, dev.smart_house.on_off.onoff, dev.smart_house.head.cmd_type);
            break;
        case E_SMART_HOUSE_CMD_SET_SMOKE_ALRAM_VAL:

            set_smoke_alarm_threshold(Swap32(dev.smart_house.smoke.alarm_val));
            dev.smart_house.head.response = SMART_HOUSE_RESPONSE_SUCCESS;
            printf(LOG_SH_TAG_NET_DATA"set smoke alarm threshold:%d\n", __LINE__, sh->smoke_alarm_threshold);
            printf(LOG_SH_TAG_INFO"烟雾报警阈值：%d ppm\r\n", __LINE__, sh->smoke_alarm_threshold);
            smart_house_msg_send_to_net(&dev, E_SMART_HOUSE_CMD_SET_SMOKE_ALRAM_VAL);
            break;
        case E_SMART_HOUSE_CMD_SET_BRIGHTNESS_VAL:

            set_brightness_threshold(Swap32(dev.smart_house.brightness.alarm_val));
            dev.smart_house.head.response = SMART_HOUSE_RESPONSE_SUCCESS;
            printf(LOG_SH_TAG_NET_DATA"set brightness threshold:%d\n", __LINE__, sh->brightness_threshold);
            printf(LOG_SH_TAG_INFO"亮度阈值：%d lux\r\n", __LINE__, sh->brightness_threshold);
            smart_house_msg_send_to_net(&dev, E_SMART_HOUSE_CMD_SET_BRIGHTNESS_VAL);
            break;
        case E_SMART_HOUSE_CMD_RESET:

            dev.smart_house.head.response = SMART_HOUSE_RESPONSE_SUCCESS;
            smart_house_msg_send_to_net(&dev, E_SMART_HOUSE_CMD_RESET);
            LOS_Msleep(500);
            printf(LOG_SH_TAG_NET_DATA"reset smart house\n", __LINE__);
            // send_to_su03t_msg(E_SOUND_TYPE_RESET, 2000);
            wait_wdt_reboot();
            break;
        default:
            printf(LOG_SH_TAG_NET_DATA"cmd no such type:%04x=============\n\n", __LINE__, dev.smart_house.head.cmd_type);
            // dev.smart_house.head.response = SMART_HOUSE_RESPONSE_FAILURE;
            // smart_house_msg_send_to_net(&dev, E_SMART_HOUSE_CMD_MAX);
            break;
    }
}

void sensor_lamp_ctrl(int onoff)
{
    smart_house_t *sh = &m_sh;
    Sensor_data_t *sd = sh->sensor_data;

    lamp_on_off(onoff);
    sd->lamp_flag = onoff;
}

void uart_rev_data_format(char * d, int len)
{
    if(d == NULL || len == 0) return;

    int cflag = 0, cnt = 0;

    for(int i = 0; i < len; ++i)
    {
        if(d[i] >= ' ' && d[i] < '~')
        {
            if(d[i] != ' ') cflag    = 1;
            if(cflag == 1)  d[cnt++] = d[i];
        }
    }
    d[cnt++] = 0x0d;
    d[cnt++] = 0x0a;
}

void uart_rev_process(void)
{
    uart_rev_t rev = {0};

    if(FifoGet(&m_uart_fifo, &rev) == 1)
    {        
        uart_rev_data_format(rev.txt, rev.len);
        printf(LOG_SH_TAG_UART"-------uart rev len:%d\r\n", __LINE__, rev.len);
        printf(LOG_SH_TAG_UART"%s\r\n", __LINE__, rev.txt);
        // dump_hex(printf, __FUNCTION__, rev.txt, rev.len);

        if(strncmp(rev.txt, CMPSTRLEN(OPEN_DOOR)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_OPEN_DOOR);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(CLOSE_DOOR)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_CLOSE_DOOR);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(OPEN_WINDOW)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_OPEN_WINDOW);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(CLOSE_WINDOW)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_CLOSE_WINDOW);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(OPEN_GARAGE_DOOR)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_OPEN_GARAGE_DOOR);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(CLOSE_GARAGE_DOOR)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_CLOSE_GARAGE_DOOR);
        }
        // else if(strncmp(rev.txt, CMPSTRLEN(STR_MUTE)) == 0)
        // {
        //     SU03T_Ctrl(E_SOUND_TYPE_MUTE);
        // }
        else if(strncmp(rev.txt, CMPSTRLEN(OPEN_LAMP)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_OPEN_LAMP);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(CLOSE_LAMP)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_CLOSE_LAMP);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(OPEN_ALL)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_OPEN_ALL);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(CLOSE_ALL)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_CLOSE_ALL);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(SKIP_HEAT)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_SKIP_HEAT);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(PPM_CALIB)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_PPM_CALIB);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(PPMUP)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_PPMUP);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(PPMDOWN)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_PPMDOWN);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(LIGHTUP)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_LIGHTUP);
        }
        else if(strncmp(rev.txt, CMPSTRLEN(LIGHTDOWN)) == 0)
        {
            SU03T_Ctrl(E_SOUND_TYPE_LIGHTDOWN);
        }
    }
}


void lcd_reset(lcd_param_t lcd)
{
    lcd_fill(0, 0, lcd.w, lcd.h, lcd.b_color);
    lcd_show_picture((lcd.w-IMAGE_W_SIEZ)/2, 0, IMAGE_W_SIEZ, IMAGE_H_SIEZ, gImage_lingzhi);
}

void lcd_icon_show(int x, int y, int *cnt, int flag, const unsigned char *picture)
{
    if(flag)
    {
        if(++(*cnt)%2 == 0)
        {
           lcd_show_picture(x, y, 32, 32, picture);
        }
        else
        {
            Printf_Lcd(x, y, LCD_LBBLUE, LCD_LGRAY, 32, 0, "  ");
        }
    }
    else
    {
        Printf_Lcd(x, y, LCD_LBBLUE, LCD_LGRAY, 32, 0, "  ");
    }
}

void lcd_picture_show(lcd_param_t lcd, Sensor_data_t s)
{
    smart_house_t  *sh = &m_sh;
    delay_ctrl_t   *dc = sh->dctrl;
    static int fire = 0, somke = 0, rain = 0, light = 0;
    static uint32_t cnt = 0, cnt1 = 0;

    lcd_icon_show(0,  lcd.start_line+lcd.f_size*5, &fire,  s.fire_flag == 0, gImage_fire3232);
    lcd_icon_show(32, lcd.start_line+lcd.f_size*5, &somke, (s.ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0)), gImage_smoke3232);
    lcd_icon_show(64, lcd.start_line+lcd.f_size*5, &rain,  s.rain == 0, gImage_rain3232);
    lcd_icon_show(96, lcd.start_line+lcd.f_size*5, &light, s.luminance < sh->brightness_threshold, gImage_night3232);

    if(s.fire_flag == 0 && !(s.ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0)) && (s.rain != 0) && (s.luminance >= sh->brightness_threshold))
    {
        Printf_Lcd(128, lcd.start_line+lcd.f_size*5, LCD_RED, LCD_LGRAY, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "   火焰报警");
    }
    else if(s.ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0) && (s.fire_flag != 0) && (s.rain != 0) && (s.luminance >= sh->brightness_threshold))
    {
        Printf_Lcd(128, lcd.start_line+lcd.f_size*5, LCD_RED, LCD_LGRAY, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "   烟雾报警");
    }
    else if(s.rain == 0 && (s.fire_flag != 0) && !(s.ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0)) && (s.luminance >= sh->brightness_threshold))
    {
        Printf_Lcd(128, lcd.start_line+lcd.f_size*5, LCD_DARKBLUE, LCD_LGRAY, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "   雨滴提醒");
    }
    else if(s.luminance < sh->brightness_threshold && (s.fire_flag != 0) && !(s.ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0)) && (s.rain != 0))
    {
        Printf_Lcd(128, lcd.start_line+lcd.f_size*5, LCD_DARKBLUE, LCD_LGRAY, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "   天黑提醒");
    }
    else if((s.fire_flag != 0) && !(s.ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0)) && (s.rain != 0) && (s.luminance >= sh->brightness_threshold))
    {
        cnt1 = 0;
        Printf_Lcd_Line_Move(0, lcd.start_line+lcd.f_size*5, LCD_BROWN, LCD_GBLUE, lcd.f_size, &cnt, 
        "     欢迎使用 智慧小屋!  语音唤醒词:\"小凌派\" 或 \"小凌小凌\"  "\
        "语音跳过烟感预热可以说:\"跳过预热\" 重新校准烟感可以说:\"烟雾校准\" 语音播报传感器值可以说:\"温度\",\"湿度\",\"亮度\",\"烟雾检测\""\
        "  ");
    }
    else
    {
        Printf_Lcd_Line_Move(128, lcd.start_line+lcd.f_size*5, LCD_BROWN, LCD_GBLUE, lcd.f_size, &cnt1, "     %s %s %s %s  ",
        s.fire_flag == 0?                                       "   火焰报警":"",
        (s.ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0))?"烟雾报警":"",
        s.rain == 0?                                            "雨滴提醒":"",
        s.luminance < sh->brightness_threshold?                 "天黑提醒":""
        );  
    }
}


// 更新需要播放的语音值 如温度,湿度,亮度,ppm
void send_to_su03t_data(Sensor_data_t sd)
{
    su03t_uart_upmsg_t uart_tx = {.head = 0};

    uart_tx.head    = SU03T_UART_MSG_HEAD;
    uart_tx.tail    = SU03T_UART_MSG_TAIL;
    uart_tx.msg_num = E_SOUND_TYPE_UPVAL;
    uart_tx.tmp     = sd.temperature;
    uart_tx.lum     = sd.humidity;
    uart_tx.light   = sd.luminance;
    uart_tx.ppm     = sd.ppm;
    // dump_hex(printf, __FUNCTION__, &uart_tx, sizeof(uart_tx));
    su03t_play((uint8_t*)&uart_tx, sizeof(uart_tx));
}

//立刻发送
void send_to_su03t_msg(SOUND_TYPE_E msg_num, uint32_t delay_ms)
{
    sound_play_t sp = {0};    

    sp.txt.head    = SU03T_UART_MSG_HEAD;
    sp.txt.tail    = SU03T_UART_MSG_TAIL;
    sp.txt.msg_num = msg_num;
    // dump_hex(printf, __FUNCTION__, &sp.txt, sizeof(sp.txt));
    su03t_play((uint8_t*)&sp.txt, sizeof(sp.txt));
    LOS_Msleep(delay_ms);
}

void Sound_play(SOUND_TYPE_E msg_num, uint32_t delay_ms, char *txt)
{
    sound_play_t    sp = {0};
    // smart_house_t  *sh = &m_sh;
    
    sp.delay       = delay_ms;
    sp.txt.head    = SU03T_UART_MSG_HEAD;
    sp.txt.tail    = SU03T_UART_MSG_TAIL;
    sp.txt.msg_num = msg_num;
    // printf(LOG_SH_TAG_SOUND"[%s]%s\r\n", __LINE__, __FUNCTION__, sh->sound_str[msg_num]);
    // dump_hex(printf, __FUNCTION__, &sp, sizeof(sp));
    FifoPut(&m_sound_fifo, &sp);
}

void SU03T_Ctrl(uint8_t read_buf)
{
    smart_house_t  *sh = &m_sh;
    delay_ctrl_t   *dc = sh->dctrl;
    Control_data_t *cd = sh->control_data;

    if(read_buf < E_SOUND_MAX && sh->sound_str[read_buf])
        printf(LOG_SH_TAG_SOUND"ctrl %d,%s\r\n", __LINE__, read_buf, sh->sound_str[read_buf]);

    switch (read_buf)
    {
    case E_SOUND_TYPE_OPEN_DOOR:
        door_open();
        sh->door.onoff   = 1;
        cd->Door_Control = 1;
        dc->door         = DELAY_TIME_MS;
        break;
    case E_SOUND_TYPE_CLOSE_DOOR:
        door_close();
        sh->door.onoff   = 0;
        break;
    case E_SOUND_TYPE_OPEN_WINDOW:
        window_open();
        sh->windows.onoff  = 1;
        cd->voice_window   = 1;
        cd->Window_Control = 1;
        break;
    case E_SOUND_TYPE_CLOSE_WINDOW:
        window_close();
        sh->windows.onoff  = 0;
        cd->voice_window   = 1;
        cd->Window_Control = 0;
        break;
    case E_SOUND_TYPE_OPEN_GARAGE_DOOR:
        car_door_open();
        sh->cardoor.onoff = 1;
        cd->voice_car     = 1;
        cd->Car_Control   = 1;
        break;
    case E_SOUND_TYPE_CLOSE_GARAGE_DOOR:
        car_door_close();
        sh->cardoor.onoff = 0;
        cd->voice_car     = 1;
        cd->Car_Control   = 0;
        break;
    // case E_SOUND_TYPE_MUTE:
    //     SYN_FrameInfo(0, "[v0] ", strlen("[v0] "));
    //     break;
    case E_SOUND_TYPE_OPEN_LAMP:
        sensor_lamp_ctrl(1);
        dc->lamp = 100;
        break;
    case E_SOUND_TYPE_CLOSE_LAMP:
        sensor_lamp_ctrl(0);
        dc->lamp = 100;
        break;
    case E_SOUND_TYPE_OPEN_ALL:
        door_open();
        sh->door.onoff   = 1;
        cd->Door_Control = 1;
        LOS_Msleep(200);

        window_open();
        sh->windows.onoff  = 1;
        cd->voice_window   = 1;
        cd->Window_Control = 1;
        LOS_Msleep(200);

        car_door_open();
        sh->cardoor.onoff = 1;
        cd->voice_car     = 1;
        cd->Car_Control   = 1;

        sensor_lamp_ctrl(1);
        dc->lamp = 100;
        break;
    case E_SOUND_TYPE_CLOSE_ALL:
        door_close();
        sh->door.onoff = 0;

        LOS_Msleep(200);

        window_close();
        sh->windows.onoff  = 0;
        cd->voice_window   = 1;
        cd->Window_Control = 0;
        LOS_Msleep(200);

        car_door_close();
        sh->cardoor.onoff = 0;
        cd->voice_car     = 1;
        cd->Car_Control   = 0;

        sensor_lamp_ctrl(0);
        dc->lamp          = 100;
        break;
    case E_SOUND_TYPE_SKIP_HEAT:
        dc->smoke         = 0;
        break;
    case E_SOUND_TYPE_PPM_CALIB:
        e53_iss_mq2_ppm_calibration();
        break;
    case E_SOUND_TYPE_PPMUP:
        up_smoke_alarm_threshold(SMOKE_STEP_VALUE);
        break;
    case E_SOUND_TYPE_PPMDOWN:
        down_smoke_alarm_threshold(SMOKE_STEP_VALUE);
        break;
    case E_SOUND_TYPE_LIGHTUP:
        up_brightness_threshold(LIGHT_STEP_VALUE);
        break;
    case E_SOUND_TYPE_LIGHTDOWN:
        down_brightness_threshold(LIGHT_STEP_VALUE);
        break;
    default:
        break;
    }
}

void base_time_handle(UINT32 para)
{
    smart_house_t  *sh       = (smart_house_t*)para;
    delay_ctrl_t   *dc       = sh->dctrl;
    Control_data_t *cd       = sh->control_data;
    uint8_t        auto_key  = 0;
    uint8_t        KeyNum    = ' ';
    static int     first_cnt = 0;
    static uint8_t t[4]      = {0}, KeyNumLast = 0;

    KeyNum = Touch_KeyScan();

    if(dc->no_key > 0)
    {//无按键
        --dc->no_key;
    }
    else if(sh->input_count != 0)
    {// 超时
        KeyNum   = '#';
        auto_key = 1;
    }

    if(KeyNum != KeyNumLast)
    {
        dc->no_key = DELAY_TIME_MS*2;
        if (KeyNum > 0 && KeyNum >= '0')
        {
            if (sh->input_count < 4)
            { //如果输入次数小于4
                //输入数字0 ~ 9 a ~ d
                KeyNum = KeyNum <= '9' ? KeyNum-'0':KeyNum-'a'+10;
                sh->key_val[sh->input_count] = KeyNum;
                sh->password <<= 4;              //使密码左移一位
                sh->password += KeyNum;          //获得一位密码
                sh->input_count++;               //计数加一
                printf(LOG_SH_TAG_SWTMR"k%d:%x,%x\r\n", __LINE__, sh->input_count, KeyNum, sh->password);
            }
        }
        else if (KeyNum == '#')
        {                                        //当按下#时,确认
            if (sh->password == DOOR_PWD)        //对照密码
            {
                printf(LOG_SH_TAG_SWTMR"密码正确\r\n", __LINE__);
                sh->password    = 0;             //密码清零
                sh->input_count = 0;
                //开门
                door_open();
                cd->Door_Control = 1;
                if(sh->door.onoff == 0)
                {
                    sh->door.onoff = 1;
                    Sound_play(E_SOUND_TYPE_OPEN_DOOR, 2000, "门已打开");
                }
            }
            else
            {
                if(sh->password == SMOKE_CALIB)
                {
                    printf(LOG_SH_TAG_SWTMR"e53_iss_mq2_ppm_calibration\n", __LINE__);
                    e53_iss_mq2_ppm_calibration();
                    cd->Door_Control = 2;
                    sh->password     = 0; //密码清零
                    sh->input_count  = 0;
                }
                else if(sh->password == SMOKE_FINISH && dc->smoke > 0)
                {
                    printf(LOG_SH_TAG_SWTMR"stop wait mq2\n", __LINE__);
                    cd->Door_Control = 2;
                    sh->password     = 0; //密码清零
                    sh->input_count  = 0;
                    dc->smoke        = 0;
                }
                else
                {
                    printf(LOG_SH_TAG_SWTMR"密码错误 %x,count:%d\r\n", __LINE__, sh->password, sh->input_count);
                    if(sh->input_count < 4)
                    {
                        if(auto_key == 1)
                        {
                            cd->Door_Control = 2;
                            sh->password     = 0; //密码清零
                            sh->input_count  = 0;
                            printf(LOG_SH_TAG_SWTMR"密码不足四位 %x\r\n", __LINE__, sh->password);
                        }
                        Sound_play(E_SOUND_TYPE_PSWD_NOP, 2500, "密码不足四位");
                    }
                    else
                    {
                        cd->Door_Control = 2;
                        sh->password     = 0; //密码清零
                        sh->input_count  = 0;
                        Sound_play(E_SOUND_TYPE_PSWD_ERR, 2000, "密码错误");
                    }
                }
            }
        }
        else if (KeyNum == '*')
        {
            int c = sh->input_count;
            sh->input_count = (c > 0)? (--c):0;
            sh->key_val[sh->input_count] = '-';
            sh->password >>= 4;
        }
    }
    KeyNumLast = KeyNum;

    if (cd->Door_Control == 0)
    {
        #define CNT_MAX 50
        static int cnt = 0;

        if(++cnt%CNT_MAX < (CNT_MAX>>2))
        {
            t[0] = sh->key_val[0];
            t[1] = sh->key_val[1];
            t[2] = sh->key_val[2];
            t[3] = sh->key_val[3];
        }
        else if(sh->input_count >= 0)
        {
            if(sh->input_count == 0)
            {
                if(first_cnt != 0)
                {
                    --first_cnt;
                    t[sh->input_count] = ' ';
                }
            }
            else
            {
                first_cnt = DELAY_TIME_MS/2;
                t[sh->input_count] = ' ';
            }
        }
        tm1650_display(t[0], t[1], t[2], t[3]);
        dc->door = DELAY_TIME_MS;
    }
    else if (cd->Door_Control == 1)
    {
        if(dc->door > 100) tm1650_display('O', 'P', 'E', 'N');
        if (dc->door > 0 && --dc->door == 100)
        {
            t[0] = sh->key_val[0] = 'c';
            t[1] = sh->key_val[1] = 'l';
            t[2] = sh->key_val[2] = 'o';
            t[3] = sh->key_val[3] = 's';
            tm1650_display(t[0], t[1], t[2], t[3]);
            door_close();
            if(sh->door.onoff == 1)
            {
                sh->door.onoff = 0;
                Sound_play(E_SOUND_TYPE_CLOSE_DOOR, 1500, "门已关闭");
            }

        }
        else if(dc->door <= 1)
        {
            cd->Door_Control = 2; // 5s定时器自动关门
        }
    }
    else if (cd->Door_Control == 2)
    {
        for (int j = 0; j < 4; j++)
        {
            sh->key_val[j] = '-';
        }
        first_cnt        = 0;
        sh->password     = 0; //密码清零
        sh->input_count  = 0;
        cd->Door_Control = 0;
    }

    if (cd->voice_window == 1) //把控制权交给声音5s
    {
        dc->voice_window--;
        if (dc->voice_window == 0)
        {
            dc->voice_window = DELAY_TIME_MS;
            cd->voice_window = 0;
        }
    }

    if (cd->voice_car == 1)   //把控制权交给声音5s
    {
        dc->voice_car--;
        if (dc->voice_car == 0)
        {
            dc->voice_car = TIM_MS_COUNT(1500);
            cd->voice_car = 0;
        }
    }

    if(dc->window > 0)   --dc->window;

    if(dc->car_door > 0) --dc->car_door;

    if(dc->smoke > 0)    --dc->smoke;

    if(dc->lamp > 0)     --dc->lamp;
}

void uart_rev_time_handle(UINT32 para)
{
    char   reac_buf[32]   = {0};
    static char_txt   buf = {0};
    static uart_rev_t rev = {0};
    static int index = 0, time = 0;
    int len = LzUartRead(UART0_ID, (unsigned char*)reac_buf, 16); //获取串口接收到LD3320的信息

    if(++time > 20) // 200ms 无数据清空已缓存的数据
    {
        time  = 0;
        index = 0;
    }

    if(len <= 0) return;

    time = 0;
    strncpy(&buf[index], reac_buf, len);
    index += len;
    if(index >= sizeof(buf)) index = sizeof(buf)-1;

    // if(len)
    // {
    //     char_txt temp = {0};
    //     strncpy(temp, buf, index);
    //     printf(LOG_SH_TAG_UART"@rev:%d,%d(%s)\r\n", __LINE__, len, index, temp);
    // }

    for(int i = 1; i < index; ++i)
    {
        if(buf[i-1] == '\r' && buf[i] == '\n')
        {
            rev.len = i+1;
            index -= rev.len;
            if(i == 1)
            {
                i = 0;
            }
            else
            {
                memcpy(rev.txt, buf, rev.len);
                FifoPut(&m_uart_fifo, &rev);
                memset(&rev, 0, sizeof(rev));
            }
            memmove(&buf[0], &buf[rev.len], sizeof(char_txt) - rev.len);
            // printf(LOG_SH_TAG_UART"size:%d, %d,%d\r\n", __LINE__, sizeof(rev), sizeof(char_txt), rev.len);
        }
    }
    if(index >= sizeof(buf)) 
    {
        index = 0;
    }
}

void update_su03t_time_handle(UINT32 para)
{
    smart_house_t *sh = (smart_house_t*)para;
    Sensor_data_t *sd = sh->sensor_data;

    send_to_su03t_data(*sd);     //更新语音数据
}

void report_state_time_handle(UINT32 para)
{
    smart_house_t *sh = (smart_house_t*)para;
    delay_ctrl_t  *dc = sh->dctrl;
    Sensor_data_t *sd = sh->sensor_data;

    if(sh->report_flag)
        smart_house_msg_send_to_net_report_state(SMART_HOUSE_RESPONSE_SUCCESS, 
            sd->temperature, sd->humidity, sd->luminance, 
            sd->rain == 0?1:0, 
            (sd->ppm > sh->smoke_alarm_threshold && (dc->smoke <= 0))?1:0, 
            sd->fire_flag == 0?1:0);
}

void touch_key_ctrl(smart_house_t *sh)
{
    Sensor_data_t *sd = sh->sensor_data;

    if (sd->touch_key)           //如果按键按下
    {
        Sound_play(E_SOUND_TYPE_RINGTONE, 2000, "[v10][t0]soundt");
        LOS_Msleep(2000);
    }
}

void fire_smoke_ctrl(smart_house_t *sh)
{
    delay_ctrl_t  *dc = sh->dctrl;
    Sensor_data_t *sd = sh->sensor_data;

    if ((sd->ppm > sh->smoke_alarm_threshold && dc->smoke <= 0) || sd->fire_flag == 0)
    {
        if(sd->fire_flag == 0)
        {
            printf(LOG_SH_TAG_INFO"火焰报警:%d\r\n", __LINE__, sd->fire_flag);
            Sound_play(E_SOUND_TYPE_ALARM_FIRE, 2000, "火焰报警");
        }
        else
        {
            printf(LOG_SH_TAG_INFO"烟雾报警:%.1f\r\n", __LINE__, sd->ppm);
            Sound_play(E_SOUND_TYPE_ALARM_SMOKE, 2000, "烟雾报警");
        }
        Sound_play(E_SOUND_TYPE_ALARM, 1500, "[v12][t3]soundn");
        LOS_Msleep(4000);
    }
}

void lamp_ctrl(smart_house_t *sh)
{
    delay_ctrl_t  *dc = sh->dctrl;
    Sensor_data_t *sd = sh->sensor_data;
#ifdef VERSION_0
    // 光敏电阻控制led
    read_light(&sd->light);
    sensor_lamp_ctrl(sd->light>3.2?1:0);
#endif

#ifdef VERSION_1
    int state = 0;
    static int last_state = 0;

    if(sd->luminance > sh->brightness_threshold && sd->infra_red_flag == 0)
        state = 0;
    else state = 1;

    if(last_state != state)
    {
        last_state = state;
        dc->lamp   = state?100:500;       //有人1秒，无人5秒
    }

    if(dc->lamp == 0) sensor_lamp_ctrl(state);
#endif
}

void windows_ctrl(smart_house_t *sh)
{
    delay_ctrl_t   *dc = sh->dctrl;
    Sensor_data_t  *sd = sh->sensor_data;
    Control_data_t *cd = sh->control_data;

    if (cd->voice_window == 0)
    {
        if ((sd->rain == 1 && sd->luminance > sh->brightness_threshold) && 
                cd->Window_Control == 0 && dc->window < 2) //没雨且太亮了  窗户是关的  就打开窗户
        {
            dc->window = DELAY_TIME_MS/2;
            window_open();
            static uint8_t first_window_flag = 1;
            cd->Window_Control = 1;                        //窗户的状态
            if (first_window_flag == 0)                    //让第一次进入判断窗户打开不发出语音
            {
                if(sh->windows.onoff == 0)
                    Sound_play(E_SOUND_TYPE_OPEN_WINDOW, 2500, "窗户已打开");
            }
            first_window_flag = 0;
            sh->windows.onoff = 1;
        }
        else if ((sd->rain == 0 || sd->luminance <= sh->brightness_threshold))
        {
            if(cd->Window_Control == 1 && dc->window < 2)
            {
                dc->window = DELAY_TIME_MS;
                window_close();
                cd->Window_Control = 0;
                if(sh->windows.onoff == 1)
                {
                    if(sd->luminance <= sh->brightness_threshold)
                        Sound_play(E_SOUND_TYPE_CLOSE_WINDOW, 2500, "窗户已关闭");
                    else 
                        Sound_play(E_SOUND_TYPE_RAIN_CLOSE_WINDOW, 3000, "雨滴检测关窗户");
                }
                sh->windows.onoff = 0;
            }
            else if(cd->Window_Control == 0)
            {
                dc->window = DELAY_TIME_MS;
            }
        }
    }
}

void car_door_ctrl(smart_house_t *sh)
{
    delay_ctrl_t   *dc = sh->dctrl;
    Sensor_data_t  *sd = sh->sensor_data;
    Control_data_t *cd = sh->control_data;

    if (cd->voice_car == 0)
    {
        if (sd->car_flag == 1)
        {
            if(cd->Car_Control == 0 && dc->car_door < 2)
            {                 
                dc->car_door = TIM_MS_COUNT(1000);
                car_door_open();
                cd->Car_Control = 1; 
                if(sh->cardoor.onoff == 0)
                {
                    sh->cardoor.onoff = 1;
                    Sound_play(E_SOUND_TYPE_OPEN_GARAGE_DOOR, 2500, "车库已打开");
                }
            }
            else if(cd->Car_Control == 1)
            {
                dc->car_door = TIM_MS_COUNT(1000);
            }
        }
        else if ((sd->car_flag == 0) && cd->Car_Control == 1 && dc->car_door < 2)
        {
            // printf(LOG_SH_TAG_INFO"car_door:%d\r\n", __LINE__, dc->car_door);
            dc->car_door = DELAY_TIME_MS/2;
            car_door_close();
            sh->cardoor.onoff = 0;
            cd->Car_Control   = 0;
            // Sound_play(E_SOUND_TYPE_CLOSE_GARAGE_DOOR, 2500, "车库已关闭");
        }
    }
}

void My_Process(void *args)
{
    TASK_PROCESS_START_LOG();
    smart_house_t  *sh = &m_sh;
    delay_ctrl_t   *dc = sh->dctrl;

    Usart_init(9600); //串口初始化
    init_sg90();      //初始化舵机
    TTP229_init();    //触摸矩阵按键io初始化
    ToyMdelay(200);   //保持稳定，否则可能发送乱码，导致语音播报模块欢迎使用

    // send_to_su03t_msg(E_SOUND_TYPE_RESET, 2000);
    // Sound_play(E_SOUND_TYPE_RESET, 10000, sh->sound_str[E_SOUND_TYPE_RESET]);
    // Sound_play(E_SOUND_TYPE_WELCOME, 10000, "欢迎使用");
    LOS_EventRead(&sh->event, EVENT_WAIT, LOS_WAITMODE_AND, LOS_WAIT_FOREVER);
    printf(LOG_SH_TAG_INFO"欢迎使用 智慧小屋\r\n", __LINE__);
    printf(LOG_SH_TAG_INFO"烟雾报警阈值：%d ppm\r\n", __LINE__, sh->smoke_alarm_threshold);
    printf(LOG_SH_TAG_INFO"亮度阈值：%d lux\r\n", __LINE__, sh->brightness_threshold);
    printf(LOG_SH_TAG_INFO"开门密码:%x,烟雾校准:%x,停止等待预热:%x\n", __LINE__, DOOR_PWD, SMOKE_CALIB, SMOKE_FINISH);

    dc->smoke = SMOKEINIT_TIME_MS;
    while (1)
    {
        uart_rev_process();
        touch_key_ctrl(sh);
        fire_smoke_ctrl(sh);
        // lamp_ctrl(sh);
        windows_ctrl(sh);
        car_door_ctrl(sh);
        LOS_Msleep(100);
    }
}

void Read_Data_Control_Process(void *args)
{
    TASK_PROCESS_START_LOG();
    smart_house_t *sh = &m_sh;
    Sensor_data_t *sd = sh->sensor_data;

    sensor_io_init();
    LOS_Msleep(100);

    tm1650_init(E_DISPLAY_LIGHT_5);     //数码管初始化
    LOS_Msleep(1000);

    if (LOS_MuxCreate(&g_i2c_muxlock) != LOS_OK)
    {
        printf(LOG_SH_TAG_INFO"Falied to create g_i2c_muxlock\n", __LINE__);
        return;
    }
    LOS_Msleep(5000);
    sensor_read_data(sd);     //读取传感器的数据
    int ret = LOS_EventWrite(&sh->event, EVENT_WAIT);
    if (ret != LOS_OK) 
    {
        printf(LOG_SH_TAG_INFO"%s write event failed ret:0x%x\n", __LINE__, __func__, ret);
    }
    sd->ppm = 20;
    if(sh->swtmr.init_fn)  sh->swtmr.init_fn(TIM_REPORT_STATE_MS, report_state_time_handle, STR_SH_SWTMR_REPORT_STATE, (uint32_t *)&sh->swtmr);
    if(sh->swtmr.start_fn) sh->swtmr.start_fn(&sh->swtmr, STR_SH_SWTMR_REPORT_STATE);

    while (1)
    {
        sensor_read_data(sd); //读取传感器的数据
        LOS_Msleep(500);
    }
}

void Screen_process(void *args)
{
    TASK_PROCESS_START_LOG();
    smart_house_t *sh = &m_sh;
    delay_ctrl_t  *dc = sh->dctrl;
    Sensor_data_t *sd = sh->sensor_data;

    lcd_io_init();          // LCD初始化
    lcd_init(2);
    lcd.w = g_lcd_size.w;
    lcd.h = g_lcd_size.h;
    lcd_reset(lcd);
    #define STR_WELCOME        "欢迎使用"
    #define STR_SMART_HOUSE    "智慧小屋"
    int len = get_str_size((char*)STR_WELCOME, lcd.f_size);
    lcd_left_move(lcd.w-len, LCD_LEFT_MOVE_EDN(lcd.w, (char*)STR_WELCOME, lcd.f_size), lcd.start_line+lcd.f_size*1, lcd.f_color, lcd.b_color, lcd.f_size, len, STR_WELCOME); 
    lcd_right_move(0, LCD_RIGHT_MOVE_EDN(lcd.w, (char*)STR_SMART_HOUSE, lcd.f_size), lcd.start_line+lcd.f_size*2, lcd.f_color, lcd.b_color, lcd.f_size, STR_SMART_HOUSE);
    Printf_Lcd(0, lcd.start_line+lcd.f_size*0, lcd.f_color, lcd.b_color, 32, E_LCD_SCREEN_ALIGN_MID_NOP, "WiFi-UDP");
    LOS_Msleep(1000);
    LOS_EventRead(&sh->event, EVENT_WAIT, LOS_WAITMODE_AND, LOS_WAIT_FOREVER);
    lcd_reset(lcd);

    if(sh->swtmr.init_fn)  sh->swtmr.init_fn(TIM_BASE_TO_MS, base_time_handle, STR_SH_SWTMR_BASE, (uint32_t *)&sh->swtmr);
    if(sh->swtmr.start_fn) sh->swtmr.start_fn(&sh->swtmr, STR_SH_SWTMR_BASE);

    if(sh->swtmr.init_fn)  sh->swtmr.init_fn(TIM_UART_REV_MS, uart_rev_time_handle, STR_SH_SWTMR_UART_REV, (uint32_t *)&sh->swtmr);
    if(sh->swtmr.start_fn) sh->swtmr.start_fn(&sh->swtmr, STR_SH_SWTMR_UART_REV);

    if(sh->swtmr.init_fn)  sh->swtmr.init_fn(TIM_UPDATE_SU03T_MS, update_su03t_time_handle, STR_SH_SWTMR_UPDATE_SU03T, (uint32_t *)&sh->swtmr);
    if(sh->swtmr.start_fn) sh->swtmr.start_fn(&sh->swtmr, STR_SH_SWTMR_UPDATE_SU03T);
   
    while (1)
    {
        Printf_Lcd(0, lcd.start_line+lcd.f_size*0, lcd.f_color, lcd.b_color, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "湿度:%.2f%%", sd->humidity);
        if(sd->luminance < sh->brightness_threshold)
            Printf_Lcd(0, lcd.start_line+lcd.f_size*1, LCD_BLACK, lcd.b_color, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "光强:%.2flux", sd->luminance);
        else
            Printf_Lcd(0, lcd.start_line+lcd.f_size*1, lcd.f_color, lcd.b_color, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "光强:%.2flux", sd->luminance);
        Printf_Lcd(0, lcd.start_line+lcd.f_size*2, lcd.f_color, lcd.b_color, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "温度:%.2f℃", sd->temperature);
        if(dc->smoke <= 0)
        {
            if(sd->ppm >= sh->smoke_alarm_threshold)
                Printf_Lcd(0, lcd.start_line+lcd.f_size*3, LCD_RED, lcd.b_color, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "PPM:%.2f%", sd->ppm);
            else
                Printf_Lcd(0, lcd.start_line+lcd.f_size*3, lcd.f_color, lcd.b_color, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "PPM:%.2f%", sd->ppm);
        }
        else
        {
            static uint32_t cnt = 0;
            Printf_Lcd_Line_Move(0, lcd.start_line+lcd.f_size*3, LCD_RED, LCD_GBLUE, lcd.f_size, &cnt, "     烟雾传感器预热中剩余%2ds,语音命令:\"跳过预热\",\"重新校准\" ",
                dc->smoke*TIM_BASE_TO_MS/1000, SMOKE_FINISH, SMOKE_CALIB);
        }

        Printf_Lcd(0, lcd.start_line+lcd.f_size*4, lcd.f_color, lcd.b_color, 32, E_LCD_SCREEN_ALIGN_LEFT_NOP, "路灯:%s, %s", sd->lamp_flag?"打开":"关闭", sd->infra_red_flag > 0?"有人":"无人");
        lcd_picture_show(lcd, *sd);


        {
            app_msg_t m       = {0};
            uint32_t  msglen  = sizeof(app_msg_t);
            if(LOS_QueueReadCopy(g_lcd_queue, &m, &msglen, 500) != LOS_OK) continue;
            if(m.cmd.msg_type != E_MSG_CMD || m.cmd.data_type != E_DATA_WIFI_LCD || 
                (m.cmd.wifi_lcd.type != E_DATA_WIFI_SERVER_LINK && m.cmd.wifi_lcd.type != E_DATA_WIFI_SERVER_NOLINK))
                continue;
            sh->report_flag = m.cmd.wifi_lcd.type == E_DATA_WIFI_SERVER_LINK? 1:0;
        }
    }
}

void Sound_process(void *args)
{
    TASK_PROCESS_START_LOG();
    int ret         = 0;
    sound_play_t sp = {0};

    while(1)
    {
        ret = FifoGet(&m_sound_fifo, &sp);
        if(ret == 1)
        {
            // dump_hex(printf, __FUNCTION__, &sp, sizeof(sp));
            su03t_play((uint8_t*)&sp.txt, sizeof(sp.txt));
            sp.delay = sp.delay > 10000? 5000: sp.delay;
            printf(LOG_SH_TAG_SOUND"%s delay:%lums,msg_num:%d,%s\r\n", __LINE__, __FUNCTION__, sp.delay, sp.txt.msg_num, m_sh.sound_str[sp.txt.msg_num]);
            LOS_Msleep(sp.delay);
        }
        else
        {
            LOS_Msleep(200);
        }
    }
}

// 接收网络数据处理
void rev_net_data_process(void *args)
{
    TASK_PROCESS_START_LOG();
    LOS_Msleep(6);

    while(1)
    {
        {
            app_msg_data_t   m   = {0};
            uint32_t         len = sizeof(app_msg_data_t);

            if(LOS_QueueReadCopy(g_netrev_queue, &m, &len, 200) != LOS_OK) continue;
            printf(LOG_SH_TAG_NET_DATA"len:%d,dev_type:%04x, cmd_type:%04x\n", __LINE__, len, m.dev_msg.head.type, m.dev_msg.smart_house.head.cmd_type);
            net_data_response(m.dev_msg);
        }
    }
}


void Smart_House(void)
{
    TASK_PROCESS_START_LOG();
    smart_house_t *sh = &m_sh;

    if (LOS_QueueCreate("queue", QUEUE_LEN_MAX, &g_tcp_queue, 0, sizeof(app_msg_data_t)))
        printf(LOG_SH_TAG_INFO"tcp_queue Falied to create Message Queue!\n", __LINE__);
    else 
        printf(LOG_SH_TAG_INFO"tcp_queue to create Message Queue! count:%d, len:%d\n", __LINE__, QUEUE_LEN_MAX, sizeof(app_msg_data_t));

    if (LOS_QueueCreate("queue", QUEUE_LEN_MAX, &g_netrev_queue, 0, sizeof(app_msg_data_t)))
        printf(LOG_SH_TAG_INFO"netrev_queue Falied to create Message Queue!\n", __LINE__);
    else 
        printf(LOG_SH_TAG_INFO"netrev_queue to create Message Queue! count:%d, len:%d\n", __LINE__, QUEUE_LEN_MAX, sizeof(app_msg_data_t));

    if (LOS_QueueCreate("queue", QUEUE_LEN_MAX, &g_wifi_queue, 0, sizeof(app_msg_t)))
        printf(LOG_SH_TAG_INFO"wifi_queue Falied to create Message Queue!\n", __LINE__);
    else 
        printf(LOG_SH_TAG_INFO"wifi_queue to create Message Queue! count:%d, len:%d\n", __LINE__, QUEUE_LEN_MAX, sizeof(app_msg_t));

    if (LOS_QueueCreate("queue", QUEUE_LEN_MAX, &g_lcd_queue, 0, sizeof(app_msg_t)))
        printf(LOG_SH_TAG_INFO"lcd_queue Falied to create Message Queue!\n", __LINE__);
    else 
        printf(LOG_SH_TAG_INFO"lcd_queue to create Message Queue! count:%d, len:%d\n", __LINE__, QUEUE_LEN_MAX, sizeof(app_msg_t));

    get_rk2206_efuse(sh->devnum);
    // LOS_Msleep(5000);

    if (LOS_EventInit(&sh->event))
        printf(LOG_SH_TAG_INFO"Falied to create EventFlags\n", __LINE__);

    FifoInit(&m_uart_fifo);
    FifoInit(&m_sound_fifo);

    create_task(&sh->thread_id[THREAD_ID_ST],       0x2000, 18,   My_Process,                NULL, "smart house");
    create_task(&sh->thread_id[THREAD_ID_READ_SD],  0x2000, 16,   Read_Data_Control_Process, NULL, "read sensor data");
    create_task(&sh->thread_id[THREAD_ID_SCREEN],   0x2000, 17,   Screen_process,            NULL, "sh screen");
    create_task(&sh->thread_id[THREAD_ID_SOUND],    0x2000, 17,   Sound_process,             NULL, "sh sound");
    create_task(&sh->thread_id[THREAD_ID_WIFI],     0x2000, 17,   wifi_process,              NULL, "sh wifi");
    create_task(&sh->thread_id[THREAD_ID_REV_NETD], 0x2000, 19,   rev_net_data_process,      NULL, "sh rev net data");
}



APP_FEATURE_INIT(Smart_House);
