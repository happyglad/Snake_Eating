#include "bsp_beep_led.h"
#include "stm32f103_min.h"

#define LED_PORT GPIOB
#define LED_PIN  5U
#define BEEP_PORT GPIOA
#define BEEP_PIN  8U

static void gpio_set_output(GPIO_TypeDef *port, uint32_t pin)
{
    volatile uint32_t *cfg = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7U) * 4U;
    *cfg = (*cfg & ~(0xFUL << shift)) | (0x3UL << shift);
}

static void pin_high(GPIO_TypeDef *port, uint32_t pin)
{
    port->BSRR = 1UL << pin;
}

static void pin_low(GPIO_TypeDef *port, uint32_t pin)
{
    port->BRR = 1UL << pin;
}

void beep_led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
    gpio_set_output(LED_PORT, LED_PIN);
    gpio_set_output(BEEP_PORT, BEEP_PIN);
    pin_high(LED_PORT, LED_PIN);
    pin_low(BEEP_PORT, BEEP_PIN);
}

void led_on(void)
{
    pin_low(LED_PORT, LED_PIN);
}

void led_off(void)
{
    pin_high(LED_PORT, LED_PIN);
}

void raw_blink(uint8_t count)
{
    uint8_t i;
    for (i = 0; i < count; i++) {
        volatile uint32_t delay;
        led_on();
        for (delay = 0; delay < 160000UL; delay++) {
        }
        led_off();
        for (delay = 0; delay < 160000UL; delay++) {
        }
    }
}

void startup_after_systeminit_probe(void)
{
    volatile uint32_t delay;
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    gpio_set_output(LED_PORT, LED_PIN);
    for (delay = 0; delay < 1800000UL; delay++) {
    }
    pin_low(LED_PORT, LED_PIN);
    for (delay = 0; delay < 1800000UL; delay++) {
    }
}

void boot_signal(uint8_t count)
{
    uint8_t i;
    for (i = 0; i < count; i++) {
        led_on();
        delay_ms(90);
        led_off();
        delay_ms(90);
    }
    delay_ms(220);
}

void feedback_food(void)
{
    led_on();
    delay_ms(35);
    led_off();
    pin_low(BEEP_PORT, BEEP_PIN);
}

void feedback_game_over(void)
{
    uint8_t i;
    for (i = 0; i < 3U; i++) {
        led_on();
        delay_ms(120);
        led_off();
        pin_low(BEEP_PORT, BEEP_PIN);
        delay_ms(80);
    }
}
