#include "bsp_usart.h"
#include "snake_game.h"
#include "stm32f103_min.h"

static void gpio_config_usart1(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    GPIOA->CRH &= ~((0xFUL << 4U) | (0xFUL << 8U));
    GPIOA->CRH |=  (0xBUL << 4U);  /* PA9 TX: AF push-pull, 50 MHz */
    GPIOA->CRH |=  (0x4UL << 8U);  /* PA10 RX: floating input */
}

void usart1_init(void)
{
    gpio_config_usart1();

    extern uint32_t SystemCoreClock;
    USART1->BRR = SystemCoreClock / 115200UL;
    USART1->CR2 = 0;
    USART1->CR3 = 0;
    USART1->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;

    nvic_enable_irq(USART1_IRQn, 0x40U);
}

void usart1_send_char(char ch)
{
    while ((USART1->SR & USART_SR_TXE) == 0U) {
    }
    USART1->DR = (uint8_t)ch;
}

void usart1_send_string(const char *str)
{
    while (*str) {
        usart1_send_char(*str++);
    }
}

void USART1_IRQHandler(void)
{
    if (USART1->SR & USART_SR_RXNE) {
        char ch = (char)(USART1->DR & 0xFFU);
        snake_game_on_command(ch);
        usart1_send_char(ch);
    }
}
