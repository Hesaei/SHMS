#include "co2.h"
#include <string.h>


uint8_t co2_buffer[CO2_RX_BUF_SIZE]; // CO2接收缓冲
volatile uint8_t co2_rx_len;         // CO2接收长度

// 接收中断回调函数,在HAL_UARTEx_RxEventCallback中调用
void CO2_UART_Callback(uint16_t Size)
{
    /* 只有收满一整帧才认；半帧丢弃，等下一帧重新同步 */
    co2_rx_len = (Size == CO2_RX_BUF_SIZE) ? CO2_RX_BUF_SIZE : 0;

    /* 重新挂上接收（必须放在最后） */
    HAL_UARTEx_ReceiveToIdle_IT(&huart2, co2_buffer, sizeof(co2_buffer));
}

// 启动接收中断
void CO2_UART_Receive_Start(void)
{
    co2_rx_len = 0;
    memset(co2_buffer, 0, sizeof(co2_buffer));
    HAL_UARTEx_ReceiveToIdle_IT(&huart2, co2_buffer, sizeof(co2_buffer));
}

/**
 * @brief 获取CO2浓度值
 *
 * @param co2_value CO2浓度值
 * @param timeout 超时时间(ms)
 * @return uint8_t 0: 成功;  1:超时;  2:模块地址错误;  3:校验和错误
 */
/**
 * @brief 获取CO2浓度值
 *
 * @param co2_value CO2浓度值
 * @param timeout 超时时间(ms)
 * @return uint8_t 0: 成功;  1:超时;  2:模块地址错误;  3:校验和错误
 */
uint8_t CO2_get_data(uint16_t *co2_value, uint32_t timeout)
{
    uint32_t start_time = HAL_GetTick();
    uint8_t  snapshot[CO2_RX_BUF_SIZE];
    uint8_t  check_sum = 0;
    uint8_t  i;
    uint8_t  ready;

    while (1)
    {
        /* 临界区：判断"整帧就绪"并原子地把数据搬走。
         * 用快照而不是直接读 co2_buffer，避免搬到一半被新帧覆盖。 */
        __disable_irq();
        ready = (co2_rx_len == CO2_RX_BUF_SIZE) ? 1 : 0;
        if (ready)
        {
            memcpy(snapshot, co2_buffer, CO2_RX_BUF_SIZE);
            co2_rx_len = 0;              /* 取走，等下一帧 */
        }
        __enable_irq();

        if (ready)
            break;

        if (HAL_GetTick() - start_time > timeout)
            return 1;                    /* 超时 */
    }

    /* 1. 校验模块地址（第1字节为0x2C） */
    if (snapshot[0] != 0x2C)
        return 2;

    /* 2. 计算和校验 */
    for (i = 0; i < CO2_RX_BUF_SIZE - 1; i++)
        check_sum += snapshot[i];

    if (check_sum != snapshot[CO2_RX_BUF_SIZE - 1])
        return 3;

    /* 3. 解析CO2浓度值 */
    *co2_value = ((uint16_t)snapshot[1] << 8) | snapshot[2];

    return 0;
}

