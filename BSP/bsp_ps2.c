#include "bsp_ps2.h"
#include "stm32f103_min.h"

#define PS2_PORT GPIOB
#define PS2_DAT_PIN 12U /* Receiver DI/DAT -> STM32 input */
#define PS2_CMD_PIN 13U /* Receiver DO/CMD <- STM32 output */
#define PS2_CS_PIN  14U /* Receiver CS/SEL <- STM32 output */
#define PS2_CLK_PIN 15U /* Receiver CLK <- STM32 output */

#define PS2_BTN_SELECT (1U << 0)
#define PS2_BTN_START  (1U << 3)
#define PS2_BTN_UP     (1U << 4)
#define PS2_BTN_RIGHT  (1U << 5)
#define PS2_BTN_DOWN   (1U << 6)
#define PS2_BTN_LEFT   (1U << 7)

static uint8_t g_ps2_data[9];
static char g_last_cmd;
static uint32_t g_last_poll_ms;

static void ps2_delay(void)
{
    volatile uint32_t i;
    for (i = 0; i < 80U; i++) {
    }
}

static void gpio_set_input_pull(GPIO_TypeDef *port, uint32_t pin, uint8_t pull_up)
{
    volatile uint32_t *cfg = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7U) * 4U;
    *cfg = (*cfg & ~(0xFUL << shift)) | (0x8UL << shift);

    if (pull_up) {
        port->ODR |= (1UL << pin);
    } else {
        port->ODR &= ~(1UL << pin);
    }
}

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

static uint8_t pin_read(GPIO_TypeDef *port, uint32_t pin)
{
    return (port->IDR & (1UL << pin)) ? 1U : 0U;
}

static uint8_t ps2_transfer_byte(uint8_t tx)
{
    uint8_t i;
    uint8_t rx = 0;

    for (i = 0; i < 8U; i++) {
        if (tx & (1U << i)) {
            pin_high(PS2_PORT, PS2_CMD_PIN);
        } else {
            pin_low(PS2_PORT, PS2_CMD_PIN);
        }

        pin_low(PS2_PORT, PS2_CLK_PIN);
        ps2_delay();
        if (pin_read(PS2_PORT, PS2_DAT_PIN)) {
            rx |= (uint8_t)(1U << i);
        }
        pin_high(PS2_PORT, PS2_CLK_PIN);
        ps2_delay();
    }

    pin_high(PS2_PORT, PS2_CMD_PIN);
    return rx;
}

static void ps2_read_data(void)
{
    uint8_t i;
    static const uint8_t request[9] = {
        0x01U, 0x42U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
    };

    pin_high(PS2_PORT, PS2_CMD_PIN);
    pin_high(PS2_PORT, PS2_CLK_PIN);
    pin_low(PS2_PORT, PS2_CS_PIN);
    ps2_delay();

    for (i = 0; i < 9U; i++) {
        g_ps2_data[i] = ps2_transfer_byte(request[i]);
    }

    pin_high(PS2_PORT, PS2_CS_PIN);
    ps2_delay();
}

void ps2_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    gpio_set_input_pull(PS2_PORT, PS2_DAT_PIN, 1U);
    gpio_set_output(PS2_PORT, PS2_CMD_PIN);
    gpio_set_output(PS2_PORT, PS2_CS_PIN);
    gpio_set_output(PS2_PORT, PS2_CLK_PIN);

    pin_high(PS2_PORT, PS2_CMD_PIN);
    pin_high(PS2_PORT, PS2_CS_PIN);
    pin_high(PS2_PORT, PS2_CLK_PIN);

    g_last_cmd = 0;
    g_last_poll_ms = 0;
}

char ps2_scan_command(void)
{
    uint8_t buttons;
    char cmd = 0;
    uint32_t now;

    now = millis();
    if ((now - g_last_poll_ms) < 20U) {
        return 0;
    }
    g_last_poll_ms = now;

    ps2_read_data();

    if ((g_ps2_data[1] != 0x41U && g_ps2_data[1] != 0x73U) || g_ps2_data[2] != 0x5AU) {
        g_last_cmd = 0;
        return 0;
    }

    buttons = g_ps2_data[3];
    if ((buttons & PS2_BTN_START) == 0U) {
        cmd = 'S';
    } else if ((buttons & PS2_BTN_SELECT) == 0U) {
        cmd = 'B';
    } else if ((buttons & PS2_BTN_UP) == 0U) {
        cmd = 'U';
    } else if ((buttons & PS2_BTN_DOWN) == 0U) {
        cmd = 'D';
    } else if ((buttons & PS2_BTN_LEFT) == 0U) {
        cmd = 'L';
    } else if ((buttons & PS2_BTN_RIGHT) == 0U) {
        cmd = 'R';
    } else {
        g_last_cmd = 0;
        return 0;
    }

    if (cmd == g_last_cmd) {
        return 0;
    }

    g_last_cmd = cmd;
    return cmd;
}
