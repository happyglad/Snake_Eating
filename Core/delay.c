#include "stm32f103_min.h"

static volatile uint32_t g_ms_ticks;

void delay_init(void)
{
    g_ms_ticks = 0;
    SysTick->LOAD = (SystemCoreClock / 1000UL) - 1UL;
    SysTick->VAL = 0;
    SysTick->CTRL = SYSTICK_CTRL_CLKSRC | SYSTICK_CTRL_TICKINT | SYSTICK_CTRL_ENABLE;
}

uint32_t millis(void)
{
    return g_ms_ticks;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = millis();
    while ((millis() - start) < ms) {
    }
}

void SysTick_Handler(void)
{
    g_ms_ticks++;
}
