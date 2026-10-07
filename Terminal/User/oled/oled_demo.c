#include "oled_demo.h"
#include "oled.h"
#include "bmp.h"
void oled_demo(void)
{
  OLED_Init();
  OLED_Clear();
  // OLED_ShowNum(30, 30, 1024, 4, 16); // 显示数字1024
  // OLED_ShowString(0, 0, "您好陈工helloworld", 16);
  OLED_ShowString(0, 0, "光强值:", 16);
	 OLED_ShowString(0, 30, "CO2:", 16);
	 OLED_ShowString(0, 60, "Temp:", 16);
	OLED_ShowString(0, 60, "Hum:", 16);
}
