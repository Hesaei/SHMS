#include "ISR_callback.h"
#include "app_main.h"
#include "delay.h"
#include "debug.h"
#include "usart.h"
#include "llcc68_p2p.h"
#include "main.h"
#include "esp.h"


/* 引用应用层定义的串口1接收标志 */
extern uint32_t g_uart1_rx_cnt;
extern uint32_t g_uart1_rx_end;

// 定义自己的EXTI中断调用的函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY1_Pin)	// 判断是否是KEY1引脚触发的中断
    {
        delay_ms(15);  // 消抖按下瞬间的电压
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
        {
					
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_8);	// 翻转LED
       
  
        }
    }
    if (GPIO_Pin == LORA_DIO1_Pin) {
        DIO1_EXTI_Callback();    // 转发给LoRa驱动处理
    }		
}

/**
 * @brief  UART 接收完成回调（接收满指定长度数据时由 HAL 调用）
 *
 * @param  huart 触发回调的串口句柄
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	/* USART1 接收完成：置位接收结束标志，供应用层轮询 */
	if (huart->Instance == USART1)
		g_uart1_rx_end = 1;
}

/**
 * @brief  UART 接收事件回调（收到指定长度或检测到空闲时由 HAL 调用）
 *
 * @param  huart 触发回调的串口句柄
 * @param  Size  本次实际接收到的字节数
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	/* USART1 接收事件：记录字节数并置位结束标志，供应用层轮询 */
	if (huart->Instance == USART1) {
		g_uart1_rx_cnt = Size;
		g_uart1_rx_end = 1;
	}

	/* USART2 连接 ESP12-F Wi-Fi模块：交给 ESP 驱动模块处理 */
	if (huart->Instance == USART2)
		esp_rx_event_handler(Size);
}
