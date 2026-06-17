#include "stm32f103_min.h"

uint32_t SystemCoreClock = 72000000UL;

static uint8_t wait_flag_set(volatile uint32_t *reg, uint32_t mask, uint32_t timeout)
{
    while (((*reg & mask) == 0U) && timeout) {
        timeout--;
    }
    return ((*reg & mask) != 0U) ? 1U : 0U;
}

void SystemInit(void)
{
    RCC->CR |= RCC_CR_HSION;
    RCC->CFGR = 0x00000000UL;
    RCC->CR &= ~(RCC_CR_PLLON | RCC_CR_HSEON);

    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    RCC->CR |= RCC_CR_HSEON;

    if (wait_flag_set(&RCC->CR, RCC_CR_HSERDY, 0x1000UL)) {
        RCC->CFGR = RCC_CFGR_HPRE_DIV1 |
                    RCC_CFGR_PPRE1_DIV2 |
                    RCC_CFGR_PPRE2_DIV1 |
                    RCC_CFGR_PLLSRC |
                    RCC_CFGR_PLLMULL9;
        RCC->CR |= RCC_CR_PLLON;
        if (wait_flag_set(&RCC->CR, RCC_CR_PLLRDY, 0x1000UL)) {
            RCC->CFGR = (RCC->CFGR & ~0x3UL) | RCC_CFGR_SW_PLL;
            (void)wait_flag_set(&RCC->CFGR, RCC_CFGR_SWS_PLL, 0x1000UL);
            if ((RCC->CFGR & 0xCUL) == RCC_CFGR_SWS_PLL) {
                SystemCoreClock = 72000000UL;
                return;
            }
        }
    }

    RCC->CR &= ~RCC_CR_PLLON;
    RCC->CFGR &= ~0x3UL;
    SystemCoreClock = 8000000UL;
}

void nvic_enable_irq(uint32_t irq, uint8_t priority)
{
    if (irq < 32U) {
        NVIC_ISER0 = (1UL << irq);
    } else {
        NVIC_ISER1 = (1UL << (irq - 32U));
    }
    NVIC_IPR_BASE[irq] = priority;
}
