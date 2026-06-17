#include "bsp_key.h"
#include "stm32f103_min.h"

#define KEY1_PORT GPIOA
#define KEY1_PIN  0U
#define KEY2_PORT GPIOC
#define KEY2_PIN  13U
#define KEY_DEBOUNCE_MS 15U

static void gpio_set_input_pull(GPIO_TypeDef *port, uint32_t pin, uint8_t pull_up)
{
    volatile uint32_t *cfg = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7U) * 4U;
    /* Set CNF=10, MODE=00 (input with pull-up/pull-down) */
    *cfg = (*cfg & ~(0xFUL << shift)) | (0x8UL << shift);
    
    if (pull_up) {
        port->ODR |= (1UL << pin);
    } else {
        port->ODR &= ~(1UL << pin);
    }
}

static uint8_t key1_prev;
static uint8_t key2_prev;
static uint32_t last_event_ms;

static uint8_t key_is_pressed(GPIO_TypeDef *port, uint32_t pin)
{
    uint8_t state = (port->IDR & (1UL << pin)) ? 1U : 0U;
    if (port == GPIOC && pin == 13U) {
        return !state; /* KEY2 on PC13 is active-low */
    }
    return state;      /* KEY1 on PA0 is active-high */
}

void key_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPCEN;
    gpio_set_input_pull(KEY1_PORT, KEY1_PIN, 0U); /* KEY1 active-high -> pull-down */
    gpio_set_input_pull(KEY2_PORT, KEY2_PIN, 1U); /* KEY2 active-low -> pull-up */
    key1_prev = 0;
    key2_prev = 0;
    last_event_ms = 0;
}

KeyEvent key_scan_event(void)
{
    static uint8_t key1_stable = 0;
    static uint8_t key2_stable = 0;
    static uint32_t key1_changed_ms = 0;
    static uint32_t key2_changed_ms = 0;

    uint8_t key1_now = key_is_pressed(KEY1_PORT, KEY1_PIN);
    uint8_t key2_now = key_is_pressed(KEY2_PORT, KEY2_PIN);
    KeyEvent event = KEY_EVENT_NONE;
    uint32_t now = millis();

    if (key1_now != key1_prev) {
        key1_changed_ms = now;
        key1_prev = key1_now;
    }
    if (key2_now != key2_prev) {
        key2_changed_ms = now;
        key2_prev = key2_now;
    }

    if ((now - key1_changed_ms) >= KEY_DEBOUNCE_MS) {
        if (key1_now && !key1_stable) {
            event = KEY_EVENT_1;
        }
        key1_stable = key1_now;
    }
    if ((now - key2_changed_ms) >= KEY_DEBOUNCE_MS) {
        if (key2_now && !key2_stable) {
            event = KEY_EVENT_2;
        }
        key2_stable = key2_now;
    }

    return event;
}
