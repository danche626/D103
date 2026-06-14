/*
 * Copyright (C) 2021 HiHope Open Source Organization .
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "los_task.h"
#include "lz_hardware.h"
#include "robot_sg90.h"

#define  USE_PWM
#define  WINDOWNS_PWM_CLOSE_US    2510

#ifdef USE_PWM
typedef struct {
    unsigned int port;
    PwmBusIo pwm_bus;
}PwmBusInfo_t;

PwmBusInfo_t sg_pwm_car = 
{
    .port = 0,
    .pwm_bus = {
        .pwm = {
            .gpio = Car_PIN,
            .func = MUX_FUNC1,
            .type = PULL_DOWN,
            .drv = DRIVE_KEEP,
            .dir = LZGPIO_DIR_KEEP,
            .val = LZGPIO_LEVEL_KEEP
        },
        .id  = FUNC_ID_PWM0,
        .mode = FUNC_MODE_M1,
    },
};

PwmBusInfo_t sg_pwm_door = 
{
    .port = 1,
    .pwm_bus = {
        .pwm = {.gpio = Door_PIN,
            .func = MUX_FUNC1,
            .type = PULL_DOWN,
            .drv = DRIVE_KEEP,
            .dir = LZGPIO_DIR_KEEP,
            .val = LZGPIO_LEVEL_KEEP
        },
        .id  = FUNC_ID_PWM1,
        .mode = FUNC_MODE_M1,
    },
};

PwmBusInfo_t sg_pwm_window = 
{
    .port = 7,
    .pwm_bus = {
        .pwm = {
            .gpio = Window_PIN,
            .func = MUX_FUNC1,
            .type = PULL_DOWN,
            .drv = DRIVE_KEEP,
            .dir = LZGPIO_DIR_KEEP,
            .val = LZGPIO_LEVEL_KEEP
        },
        .id  = FUNC_ID_PWM7,
        .mode = FUNC_MODE_NONE,
    },
};

typedef struct
{
    PwmBusInfo_t *bus;
    int           state;
    uint32_t      cnt;
    uint32_t      timeout;
    uint32_t      duty;
    uint32_t      period;
}pwm_irq_arg_t;


static pwm_irq_arg_t m_door_pwm_arg   = {.bus = &sg_pwm_door,};
static pwm_irq_arg_t m_window_pwm_arg = {.bus = &sg_pwm_window};
static pwm_irq_arg_t m_car_pwm_arg    = {.bus = &sg_pwm_car};


static unsigned int Pwm_Init(PwmBusInfo_t *p)
{
    if (PwmIoInit(p->pwm_bus) != LZ_HARDWARE_SUCCESS)
        return LZ_HARDWARE_FAILURE;
    if (LzPwmInit(p->port) != LZ_HARDWARE_SUCCESS)
        return LZ_HARDWARE_FAILURE;

    return LZ_HARDWARE_SUCCESS;
}

static __attribute__((unused)) int Pwm_get_port_from_gpioid(GpioID id)
{
    if(id == sg_pwm_car.pwm_bus.pwm.gpio)
    {
        return sg_pwm_car.port;
    }
    else if(id == sg_pwm_door.pwm_bus.pwm.gpio)
    {
        return sg_pwm_door.port;
    }
    else if(id == sg_pwm_window.pwm_bus.pwm.gpio)
    {
        return sg_pwm_window.port;
    }
    return 12;
}

static pwm_irq_arg_t *Pwm_get_arg_from_gpioid(GpioID id)
{
    if(id == m_door_pwm_arg.bus->pwm_bus.pwm.gpio)
    {
        return &m_door_pwm_arg;
    }
    else if(id == m_window_pwm_arg.bus->pwm_bus.pwm.gpio)
    {
        return &m_window_pwm_arg;
    }
    else if(id == m_car_pwm_arg.bus->pwm_bus.pwm.gpio)
    {
        return &m_car_pwm_arg;
    }
    return NULL;
}

void pwm_isr_handle(void *args)
{
    pwm_irq_arg_t *p = (pwm_irq_arg_t*)args;
    // printf("pwm%d:%u\r\n", p->bus->port, p->cnt);
    if(++p->cnt > p->timeout)
    {
        p->state = 2;
    }
    else
    {
        LzPwmStart(p->bus->port, p->duty, p->period);
    }
}


#endif

void init_sg90(void)
{
#ifdef USE_PWM

    Pwm_Init(&sg_pwm_car);
    Pwm_Init(&sg_pwm_door);
    Pwm_Init(&sg_pwm_window);

    LzPwmRegisterIsrFunc(m_door_pwm_arg.bus->port, pwm_isr_handle, &m_door_pwm_arg);
    LzPwmRegisterIsrFunc(m_window_pwm_arg.bus->port, pwm_isr_handle, &m_window_pwm_arg);
    LzPwmRegisterIsrFunc(m_car_pwm_arg.bus->port, pwm_isr_handle, &m_car_pwm_arg);

    LzPwmEnableIsr(m_door_pwm_arg.bus->port);
    LzPwmEnableIsr(m_window_pwm_arg.bus->port);
    LzPwmEnableIsr(m_car_pwm_arg.bus->port);
    
    LzPwmSetMode(m_door_pwm_arg.bus->port, PWM_ONESHOT);
    LzPwmSetMode(m_window_pwm_arg.bus->port, PWM_ONESHOT);
    LzPwmSetMode(m_car_pwm_arg.bus->port, PWM_ONESHOT);

#else
    LzGpioInit(Window_PIN);
    PinctrlSet(Window_PIN, MUX_FUNC0, PULL_UP, DRIVE_KEEP);
    LzGpioSetDir(Window_PIN, LZGPIO_DIR_OUT);

    LzGpioInit(Car_PIN);
    PinctrlSet(Car_PIN, MUX_FUNC0, PULL_UP, DRIVE_KEEP);
    LzGpioSetDir(Car_PIN, LZGPIO_DIR_OUT);

    LzGpioInit(Door_PIN);
    PinctrlSet(Door_PIN, MUX_FUNC0, PULL_UP, DRIVE_KEEP);
    LzGpioSetDir(Door_PIN, LZGPIO_DIR_OUT);
#endif
    engine_turn_left(Window_PIN);         // 关窗
    LOS_Msleep(100);
    engine_turn_left(Door_PIN);           // 关们
    LOS_Msleep(100);
    car_door_close();
}




void set_angle(GpioID id, unsigned int duty)
{
#ifdef USE_PWM
    pwm_irq_arg_t *args = Pwm_get_arg_from_gpioid(id);
    if(args)
    {
        int to = 0;
        while(args->state == 1)
        {
            if(++to > 100) break;
            LOS_Msleep(10);
        }
        args->state   = 1; //1 运行 2 停止
        args->cnt     = 0;
        args->timeout = 15;
        args->duty    = duty*1000;
        args->period  = 20000000;
        LzPwmStart(args->bus->port, args->duty, args->period);
    }
#else
    LzGpioSetVal(id, LZGPIO_LEVEL_HIGH);
    HAL_DelayUs(duty);
    LzGpioSetVal(id, LZGPIO_LEVEL_LOW);
    HAL_DelayUs(20000 - duty);
#endif
}

void set_angle_ms(GpioID id, unsigned int duty, int ms)
{
#ifndef USE_PWM
    for (int i = 0; i < ms/20; i++)
#endif        
    {
        set_angle(id, duty);
    }
}


/*Steering gear turn left*/
void engine_turn_left(GpioID id)
{
#ifndef USE_PWM
    for (int i = 0; i < 10; i++)
#endif
    {
        if(id == Window_PIN)
            set_angle(id, WINDOWNS_PWM_CLOSE_US);
        else
            set_angle(id, 2500);
    }
}

/*Steering gear turn right*/
void engine_turn_right(GpioID id)
{
#ifndef USE_PWM
    for (int i = 0; i < 10; i++)
#endif
    {
        set_angle(id, 500);
    }
}

/*Steering gear return to middle*/
void regress_90(GpioID id)
{
#ifndef USE_PWM
    for (int i = 0; i < 10; i++)
#endif
    {
        set_angle(id, 1500);
    }
}

void door_open(void)
{
    set_angle_ms(Door_PIN, 1500, 200);
}

void door_close(void)
{
    set_angle_ms(Door_PIN, 2500, 200);
}

void window_open(void)
{
    set_angle_ms(Window_PIN, 1500, 200);
}

void window_close(void)
{
    set_angle_ms(Window_PIN, WINDOWNS_PWM_CLOSE_US, 200);
}

void car_door_open(void)
{
    // set_angle_ms(Car_PIN, 1000, 1000);  //360
    // set_angle_ms(Car_PIN, 500, 200);    //卷门
    set_angle_ms(Car_PIN, 2000, 200);
}

void car_door_close(void)
{
    // set_angle_ms(Car_PIN, 2000, 1000);
    // set_angle_ms(Car_PIN, 2500, 200);   //卷门
    set_angle_ms(Car_PIN, 500, 200);
}

