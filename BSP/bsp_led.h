#ifndef BSP_LED_H
#define BSP_LED_H

#include "stm32f103_min.h"

#define LED1_GPIO_PORT     GPIOB
#define LED1_GPIO_CLK      RCC_APB2ENR_IOPBEN
#define LED1_GPIO_PIN      5U

#define LED2_GPIO_PORT     GPIOB
#define LED2_GPIO_CLK      RCC_APB2ENR_IOPBEN
#define LED2_GPIO_PIN      0U

#define LED3_GPIO_PORT     GPIOB
#define LED3_GPIO_CLK      RCC_APB2ENR_IOPBEN
#define LED3_GPIO_PIN      1U

#define LED1_ON            digitalLo(LED1_GPIO_PORT, LED1_GPIO_PIN)
#define LED1_OFF           digitalHi(LED1_GPIO_PORT, LED1_GPIO_PIN)
#define LED2_ON            digitalLo(LED2_GPIO_PORT, LED2_GPIO_PIN)
#define LED2_OFF           digitalHi(LED2_GPIO_PORT, LED2_GPIO_PIN)
#define LED3_ON            digitalLo(LED3_GPIO_PORT, LED3_GPIO_PIN)
#define LED3_OFF           digitalHi(LED3_GPIO_PORT, LED3_GPIO_PIN)

#define LED_RED            LED1_ON; LED2_OFF; LED3_OFF
#define LED_GREEN          LED1_OFF; LED2_ON; LED3_OFF
#define LED_BLUE           LED1_OFF; LED2_OFF; LED3_ON
#define LED_YELLOW         LED1_ON; LED2_ON; LED3_OFF
#define LED_PURPLE         LED1_ON; LED2_OFF; LED3_ON
#define LED_CYAN           LED1_OFF; LED2_ON; LED3_ON
#define LED_WHITE          LED1_ON; LED2_ON; LED3_ON
#define LED_RGBOFF         LED1_OFF; LED2_OFF; LED3_OFF

#define digitalHi(p,i)     { (p)->BSRR = (1UL << (i)); }
#define digitalLo(p,i)     { (p)->BRR  = (1UL << (i)); }

void LED_GPIO_Config(void);

#endif
