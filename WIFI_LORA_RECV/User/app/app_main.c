#include "app_main.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

#include "oled.h"
#include "delay.h"
#include "llcc68_p2p.h"
#include "lora_proto.h"
#include "esp_mqtt.h"

/* 串口1（调试串口）接收变量 —— ISR_callback.c 里用 extern 引用，必须保留！ */
uint8_t  g_uart1_rx_buf[32] = {0};
uint32_t g_uart1_rx_cnt = 0;
uint32_t g_uart1_rx_end = 0;

/* 收包缓冲：LLCC68 单包最大 255 字节 */
static uint8_t g_rx_buf[LORA_PAYLOAD_LEN];

/* ---- 解帧：帧头 / 长度 / 累加和 / 数值范围 四道关，任一不过返回 0 ---- */
static uint8_t lora_frame_parse(const uint8_t *buf, uint16_t len, lora_data_t *d)
{
    uint8_t  sum = 0, i;
    uint16_t co2, h10, lux;
    int16_t  t10;

    if (len != LORA_FRAME_LEN)      return 0;
    if (buf[0] != LORA_FRAME_HEAD)  return 0;

    for (i = 1; i <= 8; i++) sum += buf[i];
    if (sum != buf[9])              return 0;

    co2 = ((uint16_t)buf[1] << 8) | buf[2];
    t10 = (int16_t)(((uint16_t)buf[3] << 8) | buf[4]);
    h10 = ((uint16_t)buf[5] << 8) | buf[6];
    lux = ((uint16_t)buf[7] << 8) | buf[8];

    /* 范围哨兵：拦住脏数据 */
    if (co2 > LORA_CO2_MAX)                        return 0;
    if (t10 < LORA_T10_MIN || t10 > LORA_T10_MAX)  return 0;
    if (h10 > LORA_H10_MAX)                        return 0;
    if (lux > LORA_LUX_MAX)                        return 0;

    d->co2  = co2;
    d->temp = (float)t10 / 10.0f;
    d->humi = (float)h10 / 10.0f;
    d->lux  = lux;
    return 1;
}

/* ---- OLED 整行输出：先拷字符串，再补空格到 16 字符，避免残影和乱码 ---- */
static void OLED_ShowLine(uint8_t y, const char *s)
{
    char buf[17];
    uint8_t i;

    /* 1. 拷贝到字符串结束或 16 字符为止 */
    for (i = 0; i < 16 && s[i] != '\0'; i++)
        buf[i] = s[i];

    /* 2. 剩下的用空格补齐（保证整行 128 像素被覆盖，不留残影） */
    while (i < 16)
        buf[i++] = ' ';

    buf[16] = '\0';

    OLED_ShowString(0, y, buf, 16);
}

void app_main(void)
{
    uint16_t rx_len = 0;
    llcc68_pkt_status_lora_t pkt_status;
    uint32_t rx_ok = 0, rx_err = 0;
    lora_data_t d;
    int8_t   last_rssi = 0;
    uint32_t last_rx_tick;
    uint32_t prev_rx_tick;
    uint32_t interval_ms = 0;
    int32_t  mqtt_rt;
    char l0[24], l1[24], l2[24], l3[24];

    memset(&d, 0, sizeof(d));

    /* ---- 1. OLED ---- */
    OLED_Init();
		HAL_GPIO_WritePin(GPIOC, LED3_Pin, GPIO_PIN_RESET);
    OLED_Clear();
    OLED_ShowString(0, 0, "GW Init...", 16);

    /* ---- 2. LoRa 初始化 ---- */
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

    /* ---- 3. MQTT 初始化：连 WiFi + 连巴法云，整套约 15 秒 ---- */
    OLED_Clear();
    OLED_ShowString(0, 0, "MQTT init...", 16);
    for (;;)
    {
        mqtt_rt = esp_mqtt_init();
        if (mqtt_rt == 0)
            break;

        printf("[MQTT] init failed rt=%ld, retry in 3s\r\n", (long)mqtt_rt);
        OLED_ShowLine(16, "MQTT retry");
        HAL_Delay(3000);
    }
    printf("[MQTT] ready\r\n");

    OLED_Clear();
    last_rx_tick = HAL_GetTick();
    prev_rx_tick = last_rx_tick;

    /* ---- 4. 主循环 ---- */
    while (1)
    {
        /* 4.1 MQTT 收下行 + 发心跳（非阻塞） */
        esp_mqtt_poll();

        /* 4.2 LoRa 收包 */
        /* 收完一包芯片会退回待机，必须重新进 RX，否则只收得到第一包 */
        llcc68_lora_receive_mode(&llcc68_ctx, 0);

        if (llcc68_lora_receive_data(&llcc68_ctx, g_rx_buf, &rx_len,
                                     &pkt_status, 5000) == LLCC68_STATUS_OK)
        {
            if (lora_frame_parse(g_rx_buf, rx_len, &d))
            {
                rx_ok++;
                last_rssi    = pkt_status.rssi_pkt_in_dbm;
                prev_rx_tick = last_rx_tick;
                last_rx_tick = HAL_GetTick();
                interval_ms  = last_rx_tick - prev_rx_tick;

                printf("[RX] n=%lu dt=%lums CO2=%u T=%.1f H=%.1f Lux=%u rssi=%ddBm snr=%ddB\r\n",
                       (unsigned long)rx_ok, (unsigned long)interval_ms,
                       (unsigned)d.co2, d.temp, d.humi,
                       (unsigned)d.lux, (int)last_rssi,
                       (int)pkt_status.snr_pkt_in_db);

                /* 4.3 上云 */
                esp_mqtt_publish_sensor(d.co2, d.temp, d.humi, d.lux);
            }
            else
            {
                rx_err++;
                printf("[RX] frame error, len=%u err=%lu\r\n",
                       (unsigned)rx_len, (unsigned long)rx_err);
            }
        }

        /* 4.4 OLED 刷新 */
        sprintf(l0, "Lux %u lx", (unsigned)d.lux);
        sprintf(l1, "CO2 %u ppm", (unsigned)d.co2);
        sprintf(l2, "T%.1fC H%.1f%%", d.temp, d.humi);

        if ((HAL_GetTick() - last_rx_tick) > 30000)
            sprintf(l3, "NO DATA %lus",
                    (unsigned long)((HAL_GetTick() - last_rx_tick) / 1000));
        else
            sprintf(l3, "%ddBm %lus",
                    (int)last_rssi,
                    (unsigned long)(interval_ms / 1000));

        OLED_ShowLine(0,  l0);
        OLED_ShowLine(16, l1);
        OLED_ShowLine(32, l2);
        OLED_ShowLine(48, l3);
    }
}
