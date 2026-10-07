#include "ISR_callback.h"
#include "app_main.h"
#include "delay.h"
#include "debug.h"
#include "usart.h"
#include "co2.h"
#include "llcc68_p2p.h"



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


//1ms执行一次
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

    /* 判断中断来源是否为TIM3定时器 */
    if (htim->Instance == TIM3)
    {

			
    }
}

// 定义自己的UART接收完成调用的函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)    // 串口1接收定长数据触发中断
    {
        debug_UART_test2_Callback();    
    }
	
    if(huart->Instance == USART2)   
    {
			
//			CO2_UART_Callback(Size);
    }
}




// 定义自己的UART接收空闲调用的函数（相当于接收完成产生中断）
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART2) // // 串口1接收变长数据触发中断
    {
       CO2_UART_Callback(Size);
    }
}