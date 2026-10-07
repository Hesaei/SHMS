#ifndef LORA_PROTO_H
#define LORA_PROTO_H

#include <stdint.h>

/* ================= LoRa 帧格式 v1（固定 10 字节） =================
 * [0]     0xAA                     帧头
 * [1..2]  CO2   uint16 大端        ppm
 * [3..4]  T10   int16  大端        温度 x10 摄氏度（可负）
 * [5..6]  H10   uint16 大端        湿度 x10 %RH
 * [7..8]  LUX   uint16 大端        光照 lx
 * [9]     SUM   uint8              byte1..byte8 累加和
 * ================================================================ */

#define LORA_FRAME_LEN      10
#define LORA_FRAME_HEAD     0xAA

/* 有效范围哨兵 */
#define LORA_CO2_MAX        5000U    /* ppm    */
#define LORA_T10_MIN        (-400)   /* 0.1C   */
#define LORA_T10_MAX        1250     /* 0.1C   */
#define LORA_H10_MAX        1000U    /* 0.1%RH */
#define LORA_LUX_MAX        20000U   /* lx     */

/* 三路传感器读出来的物理量 */
typedef struct
{
    uint16_t co2;    /* ppm  */
    float    temp;   /* C    */
    float    humi;   /* %RH  */
    uint16_t lux;    /* lx   */
} lora_data_t;

#endif /* LORA_PROTO_H */
