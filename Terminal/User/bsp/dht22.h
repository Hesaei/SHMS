#ifndef __DHT22_H
#define __DHT22_H

#include "main.h"

/************************* 引脚配置 *************************/
// 自定义DHT22数据引脚（示例：PC51）
#define DHT22_GPIO_PORT    GPIOC
#define DHT22_GPIO_PIN     GPIO_PIN_15

/************************* 函数声明 *************************/
// 初始化DHT22引脚
void DHT22_Init(void);

/* 读取 DHT22 温湿度
 * 返回值：0=成功  1/2/3=起始握手时序失败  4=校验字节错  5=数值超量程
 * 读失败时 temp/humi 不会被改写，调用方保持上一次的有效值。 */
uint8_t DHT22_ReadData(float *temp, float *humi);
#endif
