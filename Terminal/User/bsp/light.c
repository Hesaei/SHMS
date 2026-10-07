#include "light.h"
#include "adc.h" 
#include <stdio.h>



// 变量声明
typedef struct
{
   uint32_t ohm; // 光敏电阻值
   uint16_t lux; // 勒克斯
} PhotoRes_TypeDef;

// GL5528光敏电阻的阻值与流明对应的关系
const PhotoRes_TypeDef GL5528[] =
    {
        {100000, 0}, // 接近全黑
        {70000, 1},  // 极暗环境
        {50000, 1},  // 深夜无灯
        {40000, 1},  // 昏暗角落
        {30000, 2},  // 弱光环境
        {20000, 4},  // 夜间灯光旁
        {15000, 5},  // 傍晚室内
        {10000, 10}, // 昏暗室内
        {7000, 17},  // 普通室内
        {5000, 29},  // 室内台灯旁
        {4000, 45},  // 明亮室内
        {3000, 68},  // 晴天窗边
        {2000, 124}, // 晴天户外阴影
        {1000, 350}, // 烈日 / 强光直射
};

/* ADC 满量程 12bit，注意保持 4096 —— 改成 4095 会让分母出现 0 */
#define ADC_FULL_SCALE  4096.0f
#define ADC_VREF        3.3f
/* 电压钳位上限：保证 (ADC_VREF - voltage) 恒大于 0，避免除零 */
#define LIGHT_VOLT_MAX  (ADC_VREF - 0.01f)

// 获取光敏电压；读失败返回负值
static float get_light_voltage(void)
{
    uint16_t adc_value;
    float    voltage;

    HAL_ADC_Start(&hadc1);

    /* 原来是 HAL_MAX_DELAY，ADC 配置出错就永久卡死在这 */
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc1);
        return -1.0f;
    }

    adc_value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    voltage = (float)adc_value * ADC_VREF / ADC_FULL_SCALE;


    return voltage;
}

// 遍历数组，获取光照度单位lux
uint16_t GetLux(void)
{
    static uint16_t last_lux = 0;   /* 读失败时保持上一次的有效值 */
    uint16_t lux = 0;
    float    voltage;
    uint32_t resistance;
    int      i;

    voltage = get_light_voltage();
    if (voltage < 0.0f)
        return last_lux;            /* ADC 读失败 */

    if (voltage > LIGHT_VOLT_MAX)
        voltage = LIGHT_VOLT_MAX;   /* 钳位，防止分母为 0 */

    // 光敏电阻值
    resistance = (uint32_t)(10.0f * 1000.0f * voltage / (ADC_VREF - voltage));

    // 根据电阻值得出光照度（表按 ohm 降序，必须保持这个顺序）
    for (i = 0; i < (int)(sizeof(GL5528) / sizeof(GL5528[0])); i++)
    {
        lux = GL5528[i].lux;
        if (resistance >= GL5528[i].ohm)
            break;
    }

    last_lux = lux;
    return lux;
}

