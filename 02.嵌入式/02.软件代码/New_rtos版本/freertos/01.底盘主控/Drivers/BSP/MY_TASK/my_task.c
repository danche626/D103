#include "my_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "stdio.h"

/* 硬件驱动头文件 */
#include "imu.h"
#include "nrf24L01.h"
#include "oled.h"
#include "led.h"
#include "data.h"
#include "usart.h"

/* 任务句柄 */
TaskHandle_t StartTask_Handler;
TaskHandle_t DATA_GET_Task_Handler;
TaskHandle_t DATA_COMM_Handler;
TaskHandle_t OLED_SHOW_Task_Handler;
TaskHandle_t PRINT_Task_Handler;
TaskHandle_t DATA_Exchange_Handler;
TaskHandle_t UART_Trans_Handler;

void create_task(void)
{
    xTaskCreate((TaskFunction_t )start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &StartTask_Handler);
}

void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();

//    xTaskCreate(print_task,    "print",    PRINT_STK_SIZE,    NULL, PRINT_PRIO,      &PRINT_Task_Handler);
    xTaskCreate(data_get_task, "imu_get",  DATA_GET_STK_SIZE, NULL, DATA_GET_PRIO,   &DATA_GET_Task_Handler);
	xTaskCreate(data_exchange_task, "data_exchange",  DATA_EXCHANGE_SIZE, NULL, DATA_EXCHANGE_PRIO,   &DATA_Exchange_Handler);
    xTaskCreate(DATA_COMM_task,"nrf_comm", DATA_COMM_SIZE,    NULL, DATA_COMM_PRIO,  &DATA_COMM_Handler);
	xTaskCreate(UART_Trans_task,"UART_Trans", UART_Trans_SIZE,    NULL, UART_Trans_PRIO,  &UART_Trans_Handler);
//  xTaskCreate(oled_show_task,"oled",     OLED_SHOW_STK_SIZE,NULL, OLED_SHOW_PRIO,  &OLED_SHOW_Task_Handler);

    vTaskDelete(NULL);
    taskEXIT_CRITICAL();
}

/**
 * @brief 获取陀螺仪数据
 */
void data_get_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        imu_updata(); 
        pid_calculate();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}
/**
 * @brief 串口通讯数据
 */
void UART_Trans_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
		uart_sand();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}
/**
 * @brief 数据交换任务
 */
void data_exchange_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        data_change();

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}
/**
 * @brief 无线通讯任务
 */
void DATA_COMM_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while(1)
    {
		if(nrf24l01_rx_packet(C_rx_buf) == 0){}
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2));
    }
}

/**
 * @brief OLED显示任务 (50ms周期)
 */
void oled_show_task(void *pvParameters)
{


    TickType_t xLastWakeTime = xTaskGetTickCount(); // 改用精准延时
    while(1)
    {
	

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50)); 
    }
}

/**
 * @brief 调试打印任务 (50ms周期) - 快照机制防止乱码
 */
void print_task(void *pvParameters)
{
		TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1) 
    {
		data_print();
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}
