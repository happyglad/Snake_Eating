#include "bsp_led.h"

static void gpio_set_output(GPIO_TypeDef *port, uint32_t pin)
{
    volatile uint32_t *cfg = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7U) * 4U;
    *cfg = (*cfg & ~(0xFUL << shift)) | (0x3UL << shift);
}

void LED_GPIO_Config(void)
{
    RCC->APB2ENR |= LED1_GPIO_CLK | LED2_GPIO_CLK | LED3_GPIO_CLK;
    gpio_set_output(LED1_GPIO_PORT, LED1_GPIO_PIN);
    gpio_set_output(LED2_GPIO_PORT, LED2_GPIO_PIN);
    gpio_set_output(LED3_GPIO_PORT, LED3_GPIO_PIN);

    LED_RGBOFF;
}
