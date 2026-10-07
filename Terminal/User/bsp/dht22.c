#include "dht22.h"
#include "app_main.h"
#include "delay.h"
#include <stdio.h>

/************************* 私有函数声明 *************************/
// 设置引脚为输出模式
static void DHT22_SetOutputMode(void);

// 设置引脚为输入模式
static void DHT22_SetInputMode(void);

// 发送起始信号
static uint8_t DHT22_SendStartSignal(void);

// 读取一个字节数据
static uint8_t DHT22_ReadByte(void);

/************************* 引脚模式配置 *************************/
static void DHT22_SetOutputMode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = DHT22_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
}

static void DHT22_SetInputMode(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = DHT22_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;     // 输入模式
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
}


// 这个用STM32CubeMX配置了  相关的配置在gpio.c当中
void DHT22_Init(void)
{
    // __HAL_RCC_GPIOA_CLK_ENABLE();  // 使能GPIOA时钟（根据实际引脚修改）
    
    // DHT22_SetOutputMode();
    // HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_GPIO_PIN, GPIO_PIN_SET);  // 空闲状态为高电平
	
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
	__HAL_RCC_GPIOC_CLK_ENABLE();
	
	HAL_GPIO_WritePin(GPIOC, LED3_Pin|DHT22_GPIO_Output_Pin, GPIO_PIN_SET);
	
  GPIO_InitStruct.Pin = DHT22_GPIO_Output_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(DHT22_GPIO_Output_GPIO_Port, &GPIO_InitStruct);	
	

	
	
}

uint8_t DHT22_ReadData(float *temp, float *humi)
{
    uint8_t  ret;
    uint8_t  i;
    uint8_t  data[5] = {0};
    uint16_t h10;
    int16_t  t10;

    /* 起始握手：失败就直接返回，不要拿垃圾数据去算 */
    ret = DHT22_SendStartSignal();
    if (ret != 0)
        return ret;

    /* 连续读 5 个字节 */
    for (i = 0; i < 5; i++)
        data[i] = DHT22_ReadByte();

    /* 校验字节 = 前 4 字节之和的低 8 位 */
    if (data[4] != (uint8_t)((data[0] + data[1] + data[2] + data[3]) & 0xFF))
        return 4;

    /* 湿度：第 1、2 字节，大端，单位 0.1%RH */
    h10 = ((uint16_t)data[0] << 8) | data[1];

    /* 温度：第 3、4 字节，最高位为 1 表示负温度 */
    t10 = (int16_t)(((uint16_t)(data[2] & 0x7F) << 8) | data[3]);
    if (data[2] & 0x80)
        t10 = -t10;

    /* 量程检查：超范围说明数据不可信 */
    if (h10 > 1000)                    /* 0 ~ 100.0 %RH   */
        return 5;
    if (t10 < -400 || t10 > 1250)      /* -40.0 ~ 125.0 C */
        return 5;

    *humi = (float)h10 / 10.0f;
    *temp = (float)t10 / 10.0f;

    return 0;
}

/************************* 私有函数实现 *************************/
//启动信号
static uint8_t DHT22_SendStartSignal(void)
{
	uint16_t time_count = 0;
	
	//配置引脚为输出模式
	DHT22_SetOutputMode();
	//开始信号前，保证引脚先为高电平
	HAL_GPIO_WritePin(GPIOC, LED3_Pin|DHT22_GPIO_Output_Pin, GPIO_PIN_SET);
	delay_us(100);
	//拉低至少500us
	HAL_GPIO_WritePin(GPIOC, LED3_Pin|DHT22_GPIO_Output_Pin, GPIO_PIN_RESET);	
	delay_us(800);
	//拉高20~40us
	HAL_GPIO_WritePin(GPIOC, LED3_Pin|DHT22_GPIO_Output_Pin, GPIO_PIN_SET);	
	delay_us(30);

	//引脚配置为输入模式
	DHT22_SetInputMode();
	
	
	time_count = 0;
	//等待低电平到来
	
	while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_SET)
	{
		time_count++;
		delay_us(2);
		if(time_count >=200) //等待400us，未有低电平则返回
			return 1;
	}
	//程序到这里，已经检测到低电平
	
	time_count = 0;
	//等待高电平到来，过滤低电平（这个代码，检测低电平，则死循环；检测到高电平，退出循环）
	while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_RESET)
	{
		time_count++;
		delay_us(2);
		if(time_count >=200) //等待400us，未有高电平则返回  等待400us,足够过滤低电平
			return 2;
	}	
	
	time_count = 0;
	//等待低电平到来，过滤高电平（这个代码，检测高电平，则死循环；检测到低电平，退出循环）
	while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_SET)
	{
		time_count++;
		delay_us(2);
		if(time_count >=200) //等待400us，未有低电平则返回  等待400us,足够过滤高电平
			return 3;
	}	
		
	return 0;
}

static uint8_t DHT22_ReadByte(void)
{
  uint8_t i, time_count = 0,  rxdata = 0x00; //0 0 1 0 0 0 0 0 

	for(i=0; i<8; i++) 
	{
		time_count = 0;
		//等待高电平到来，过滤低电平（这个代码，检测低电平，则死循环；检测到高电平，退出循环）
		while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_RESET)
		{
			time_count++;
			delay_us(2);
			if(time_count >=200) //等待400us，未有高电平则返回  等待400us,足够过滤低电平
				return 0x00;
		}			
	  //延时40微秒，再判断
		delay_us(40);
		//成立表示数据位为1
		if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_SET)
		{
			rxdata |= (0x01<<(7-i));
			
			//在这里，波形还有高电平
			time_count = 0;
			//等待低电平到来，过滤高电平（这个代码，检测高电平，则死循环；检测到低电平，退出循环）
			while(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15) == GPIO_PIN_SET)
			{
				time_count++;
				delay_us(2);
				if(time_count >=200) //等待400us，未有低电平则返回  等待400us,足够过滤高电平
					return 0;
			}			
		}
		else
		{
			rxdata &= ~(0x01<<(7-i));
		
		}
		
		
	}
	return rxdata;
	
}