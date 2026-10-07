#ifndef DEBUG_H
#define DEBUG_H

#include "main.h"

#define DATA_SIZE 6

extern uint8_t g_rx_flag;
extern uint8_t g_buff[DATA_SIZE];

void Debug_UART_Receive_Start(void);
void debug_test2(void);

void debug_UART_test2_Callback(void);
#endif // DEBUG_H