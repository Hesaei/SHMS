#ifndef APP_MAIN_H	// 防止头文件重复包含
#define APP_MAIN_H
#include "main.h"


#define DMA_CH    1   //1 使用DMA   0不使用DMA

extern uint16_t adc1_values[2];

void app_main(void);
#endif