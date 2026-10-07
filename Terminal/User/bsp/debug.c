#include "debug.h"
#include "usart.h"



//变量定义
uint8_t g_rx_flag = 0;
uint8_t g_buff[DATA_SIZE]={0};

//轮循查找是否有数据要接收
void debug_test1(void)
{


	while(1)
	{
		// 指定串口1接收10字节数据，HAL_MAX_DELAY阻塞永久等待，
		// 直到接收到数据。如果接收到数据，则返回HAL_OK    。
		// 如果你们不希望阻塞，可以指定一个超时时间，如1000 (1000ms)。
		if(HAL_UART_Receive(&huart1, g_buff, DATA_SIZE, HAL_MAX_DELAY) == HAL_OK)
		{
			// 把收到的数据原封不动的发出去, HAL_MAX_DELAY阻塞永久等待
			// 你也可以单独调用该函数主动发送你需要的数据
			///HAL_UART_Transmit(&huart1, g_buff, DATA_SIZE, HAL_MAX_DELAY);
			
		}
	
	}
}

// 这个函数被HAL_UART_RxCpltCallback()调用
void debug_UART_test2_Callback(void)
{
   
    g_rx_flag = 1;
    // 重新用中断的方式接收10个字节的数据，继续接收
    HAL_UART_Receive_IT(&huart1, g_buff, DATA_SIZE);
	  
}

void Debug_UART_Receive_Start(void)
{
	//启动开启接收中断--指定接受数据长度
	HAL_UART_Receive_IT(&huart1, g_buff, DATA_SIZE);

}

