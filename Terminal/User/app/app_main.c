#include "app_main.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

#include "co2.h"
#include "light.h"
#include "dht22.h"
#include "delay.h"
#include "llcc68_p2p.h"
#include "lora_proto.h"

/* ---------------- 周期配置 ---------------- */
#define SEND_PERIOD_MS    3000   /* LoRa 上报周期          */
#define CO2_PERIOD_MS     2000   /* CO2 采样周期           */
#define DHT_PERIOD_MS     2500   /* DHT22 采样周期（须>=2s）*/
#define LIGHT_PERIOD_MS   1000   /* 光照采样周期           */
#define CO2_TIMEOUT_MS    300    /* CO2 单次等待超时        */

/* ---------------- 组帧：10 字节 ---------------- */
static uint8_t lora_frame_pack(uint8_t *buf, const lora_data_t *d)
{
    int16_t  t10 = (int16_t)(d->temp * 10.0f);
    uint16_t h10 = (uint16_t)(d->humi * 10.0f);
    uint8_t  sum = 0, i;

    buf[0] = LORA_FRAME_HEAD;
    buf[1] = (uint8_t)(d->co2 >> 8);
    buf[2] = (uint8_t)(d->co2 & 0xFF);
    buf[3] = (uint8_t)((uint16_t)t10 >> 8);
    buf[4] = (uint8_t)((uint16_t)t10 & 0xFF);
    buf[5] = (uint8_t)(h10 >> 8);
    buf[6] = (uint8_t)(h10 & 0xFF);
    buf[7] = (uint8_t)(d->lux >> 8);
    buf[8] = (uint8_t)(d->lux & 0xFF);

    for (i = 1; i <= 8; i++)
        sum += buf[i];
    buf[9] = sum;

    return LORA_FRAME_LEN;
}

void app_main(void)
{
    lora_data_t d;
    uint16_t    co2_new;
    float       t_new, h_new;

    uint8_t  frame[LORA_FRAME_LEN];
    uint8_t  ret_dht = 0, ret_co2 = 0;
    uint32_t t_send, t_co2, t_dht, t_light;

    memset(&d, 0, sizeof(d));

    /* ---- 1. 传感器启动 ---- */
    CO2_UART_Receive_Start();
    DHT22_Init();
    HAL_Delay(2000);                 /* DHT22 上电稳定，不能省 */

    /* ---- 2. LoRa 初始化：每次失败都打印，不会静默卡死 ---- */
    for (;;)
    {
        if (llcc68_init(&llcc68_ctx) == LLCC68_STATUS_OK)
        {
            printf("[INIT] LLCC68 ready\r\n");
            break;
        }
        printf("[INIT] LLCC68 init failed, retry in 2s\r\n");
        HAL_Delay(2000);
    }

    t_send = t_co2 = t_dht = t_light = HAL_GetTick();

    /* ---- 3. 主循环 ---- */
    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* 3.1 光照 */
        if (now - t_light >= LIGHT_PERIOD_MS)
        {
            t_light = now;
            d.lux = GetLux();
        }

        /* 3.2 CO2：读失败保持旧值 */
        if (now - t_co2 >= CO2_PERIOD_MS)
        {
            t_co2 = now;
            ret_co2 = CO2_get_data(&co2_new, CO2_TIMEOUT_MS);
            if (ret_co2 == 0)
                d.co2 = co2_new;
        }

        /* 3.3 温湿度：读失败保持旧值，周期 >= 2s */
        if (now - t_dht >= DHT_PERIOD_MS)
        {
            t_dht = now;
            ret_dht = DHT22_ReadData(&t_new, &h_new);
            if (ret_dht == 0)
            {
                d.temp = t_new;
                d.humi = h_new;
            }
        }

        /* 3.4 LoRa 上报 */
        if (now - t_send >= SEND_PERIOD_MS)
        {
            uint8_t len;

            t_send = now;
            len = lora_frame_pack(frame, &d);

            if (llcc68_lora_send(&llcc68_ctx, frame, len, 300) == LLCC68_STATUS_OK)
            {
                printf("[TX] CO2=%uppm T=%.1fC H=%.1f%% Lux=%ulx\r\n",
                       (unsigned)d.co2, d.temp, d.humi, (unsigned)d.lux);
                printf("[TX] %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\n",
                       frame[0], frame[1], frame[2], frame[3], frame[4],
                       frame[5], frame[6], frame[7], frame[8], frame[9]);
            }
            else
            {
                printf("[TX] lora send FAIL\r\n");
            }

            printf("[ST] dht=%u co2=%u\r\n",
                   (unsigned)ret_dht, (unsigned)ret_co2);
        }
    }
}
