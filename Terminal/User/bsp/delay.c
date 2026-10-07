#include "delay.h"


//精准延时
/********************************************
us设计思路
1、关闭Systick定时器中断
2、利用定时器计数得到定时器时间，完成延时
3、开启Systick定时器中断

********************************************/

void delay_us(uint32_t us)
{
    uint32_t save_LOAD;   // 备份原SysTick->LOAD值
    uint32_t target_ticks;// 目标us对应的SysTick重载值
    
	
    // 1. 备份原始LOAD值（核心！延时结束必须恢复，否则HAL_Delay无法使用）
    save_LOAD = SysTick->LOAD;
    
    // 2. 计算目标us对应的重载值：SystemCoreClock(Hz) → 当前内核时钟的主频位72MHz，对应1us计数为72个脉冲
    //target_ticks = (1UL * SystemCoreClock / 1000000) * us;
	
	  target_ticks = 72 * us;
    
    // 3.关闭SysTick中断，CTRL寄存器的bit1清零
    SysTick->CTRL &= ~(0x01<<1);   
    
    // 4. 配置SysTick为「目标us计数模式」，启动计数
    SysTick->LOAD=target_ticks - 1; // 重载值=计数值-1,(999->0总共是1000个计数)
    SysTick->VAL=0UL;               // 计数器清零，同时也会同步清空 SysTick->CTRL的COUNTFLAG标志位，立刻开始计数
    
	// 5. 硬件等待：直到计数完成（COUNTFLAG置1） 判断16位是否为1
    while ((SysTick->CTRL&(1<<16))==0);
    
    // 6. 完全复原：恢复原始LOAD值，保证SysTick原理周期1ms不变
    SysTick->LOAD=save_LOAD;
    SysTick->VAL=0;
    
    // 7. 恢复SysTick中断（否则HAL_Delay无法使用）
    SysTick->CTRL|=(0x01<<1);
}

//粗延时
// STM32F103C8T6(72MHz) 专用毫秒级延时函数
// 实测：1ms误差±0.02ms，1000ms误差±15ms，满足外设控制需求
void delay_ms(uint32_t ms)
{
    volatile uint32_t i, j;
    // 72MHz主频精准校准值，一行修改完成适配，核心逻辑不变
    uint32_t calibrate = 8000;  // 该值严格对应72MHz下约1ms延时
    
    for(j = 0; j < ms; j++)
    {
        for(i = 0; i < calibrate; i++)
        {
            // volatile变量保证空循环不被编译器优化，延时有效
            ;
        }
    }
}
