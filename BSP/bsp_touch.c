#include "bsp_touch.h"
#include "bsp_lcd_game.h"
#include "stm32f103_min.h"

#define TOUCH_CS_PORT      GPIOD
#define TOUCH_CS_PIN       13U
#define TOUCH_CLK_PORT     GPIOE
#define TOUCH_CLK_PIN      0U
#define TOUCH_MOSI_PORT    GPIOE
#define TOUCH_MOSI_PIN     2U
#define TOUCH_MISO_PORT    GPIOE
#define TOUCH_MISO_PIN     3U
#define TOUCH_IRQ_PORT     GPIOE
#define TOUCH_IRQ_PIN      4U

#define XPT2046_CHANNEL_X  0x90U
#define XPT2046_CHANNEL_Y  0xD0U

#define TOUCH_DEBOUNCE_MS  80U
#define TOUCH_SAMPLE_MS    20U
#define TOUCH_RAW_SWIPE_MIN 350U

#define TOUCH_BTN_BOT_Y0   272
#define TOUCH_BTN_BOT_Y1   320
#define TOUCH_BTN_COL0_X1  80
#define TOUCH_BTN_COL1_X1  160
#define TOUCH_BTN_COL2_X1  240

typedef struct {
    int16_t x;
    int16_t y;
} TouchPoint;

static uint32_t g_last_touch_ms = 0;
static uint32_t g_last_sample_ms = 0;
static uint8_t g_touch_active = 0;
static uint8_t g_invalid_active = 0;
static uint8_t g_is_playing = 0;
static uint16_t g_start_raw_x = 0;
static uint16_t g_start_raw_y = 0;
static uint16_t g_last_raw_x = 0;
static uint16_t g_last_raw_y = 0;

extern void usart1_send_string(const char *str);
extern void usart1_send_char(char ch);

static void gpio_set_cfg(GPIO_TypeDef *port, uint32_t pin, uint32_t cfg)
{
    volatile uint32_t *reg = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7U) * 4U;
    *reg = (*reg & ~(0xFUL << shift)) | (cfg << shift);
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

static void touch_delay(void)
{
    volatile uint32_t i;
    for (i = 0; i < 24U; i++) {
    }
}

static uint8_t touch_is_pressed(void)
{
    return pin_read(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN) == 0U;
}

static void touch_send_number(uint32_t num)
{
    char buf[11];
    uint8_t i = 0;

    if (num == 0U) {
        usart1_send_char('0');
        return;
    }

    while (num && i < 10U) {
        buf[i++] = (char)('0' + (num % 10U));
        num /= 10U;
    }

    while (i > 0U) {
        usart1_send_char(buf[--i]);
    }
}

static void touch_debug_point(const char *tag, uint16_t raw_x, uint16_t raw_y, const TouchPoint *point)
{
    usart1_send_string("\r\n[Touch] ");
    usart1_send_string(tag);
    usart1_send_string(" raw=");
    touch_send_number(raw_x);
    usart1_send_char(',');
    touch_send_number(raw_y);

    if (point) {
        usart1_send_string(" xy=");
        touch_send_number((uint32_t)point->x);
        usart1_send_char(',');
        touch_send_number((uint32_t)point->y);
    }
    usart1_send_string("\r\n");
}

static void touch_debug_miss(const TouchPoint *point, uint16_t raw_x, uint16_t raw_y)
{
    usart1_send_string("\r\n[Touch] miss xy=");
    touch_send_number((uint32_t)point->x);
    usart1_send_char(',');
    touch_send_number((uint32_t)point->y);
    usart1_send_string(" raw=");
    touch_send_number(raw_x);
    usart1_send_char(',');
    touch_send_number(raw_y);
    usart1_send_string(" valid START/MODE buttons only\r\n");
}

static const char *touch_event_name(TouchEvent event)
{
    switch (event) {
        case TOUCH_EVENT_UP:    return "UP";
        case TOUCH_EVENT_DOWN:  return "DOWN";
        case TOUCH_EVENT_LEFT:  return "LEFT";
        case TOUCH_EVENT_RIGHT: return "RIGHT";
        case TOUCH_EVENT_START: return "START";
        case TOUCH_EVENT_MODE:  return "MODE";
        default:                return "NONE";
    }
}

static void touch_debug_event(TouchEvent event)
{
    usart1_send_string("\r\n[Touch] event=");
    usart1_send_string(touch_event_name(event));
    usart1_send_string("\r\n");
}

static uint16_t touch_abs_diff_u16(uint16_t a, uint16_t b)
{
    return (a >= b) ? (uint16_t)(a - b) : (uint16_t)(b - a);
}

static TouchEvent touch_swipe_to_event(uint16_t start_raw_x, uint16_t start_raw_y,
                                       uint16_t end_raw_x, uint16_t end_raw_y)
{
    int16_t screen_dx;
    int16_t screen_dy;
    uint16_t abs_dx;
    uint16_t abs_dy;

    if (touch_abs_diff_u16(start_raw_x, end_raw_x) < TOUCH_RAW_SWIPE_MIN &&
        touch_abs_diff_u16(start_raw_y, end_raw_y) < TOUCH_RAW_SWIPE_MIN) {
        return TOUCH_EVENT_NONE;
    }

    /* LCD x increases when raw_y decreases; LCD y increases when raw_x decreases. */
    screen_dx = (int16_t)((int32_t)start_raw_y - (int32_t)end_raw_y);
    screen_dy = (int16_t)((int32_t)start_raw_x - (int32_t)end_raw_x);
    abs_dx = (screen_dx >= 0) ? (uint16_t)screen_dx : (uint16_t)(-screen_dx);
    abs_dy = (screen_dy >= 0) ? (uint16_t)screen_dy : (uint16_t)(-screen_dy);

    if (abs_dx > abs_dy) {
        return (screen_dx > 0) ? TOUCH_EVENT_LEFT : TOUCH_EVENT_RIGHT;
    }

    return (screen_dy > 0) ? TOUCH_EVENT_DOWN : TOUCH_EVENT_UP;
}

static void touch_debug_swipe(TouchEvent event)
{
    usart1_send_string("\r\n[Touch] swipe raw-start=");
    touch_send_number(g_start_raw_x);
    usart1_send_char(',');
    touch_send_number(g_start_raw_y);
    usart1_send_string(" raw-end=");
    touch_send_number(g_last_raw_x);
    usart1_send_char(',');
    touch_send_number(g_last_raw_y);
    usart1_send_string(" event=");
    usart1_send_string(touch_event_name(event));
    usart1_send_string("\r\n");
}

static void touch_write_cmd(uint8_t cmd)
{
    uint8_t i;

    pin_low(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN);
    pin_low(TOUCH_CLK_PORT, TOUCH_CLK_PIN);

    for (i = 0; i < 8U; i++) {
        if (cmd & (uint8_t)(0x80U >> i)) {
            pin_high(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN);
        } else {
            pin_low(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN);
        }
        touch_delay();
        pin_high(TOUCH_CLK_PORT, TOUCH_CLK_PIN);
        touch_delay();
        pin_low(TOUCH_CLK_PORT, TOUCH_CLK_PIN);
    }
}

static uint16_t touch_read_data(void)
{
    uint8_t i;
    uint16_t value = 0U;

    pin_high(TOUCH_CLK_PORT, TOUCH_CLK_PIN);
    for (i = 0; i < 12U; i++) {
        pin_low(TOUCH_CLK_PORT, TOUCH_CLK_PIN);
        touch_delay();
        value <<= 1;
        if (pin_read(TOUCH_MISO_PORT, TOUCH_MISO_PIN)) {
            value |= 1U;
        }
        pin_high(TOUCH_CLK_PORT, TOUCH_CLK_PIN);
        touch_delay();
    }
    return value;
}

static uint16_t touch_read_adc(uint8_t channel)
{
    uint16_t value;

    pin_low(TOUCH_CS_PORT, TOUCH_CS_PIN);
    touch_write_cmd(channel);
    value = touch_read_data();
    pin_high(TOUCH_CS_PORT, TOUCH_CS_PIN);

    return value;
}

static uint8_t touch_read_raw(uint16_t *raw_x, uint16_t *raw_y)
{
    uint16_t x1;
    uint16_t y1;
    uint16_t x2;
    uint16_t y2;
    uint16_t dx;
    uint16_t dy;

    if (!touch_is_pressed()) {
        return 0U;
    }

    x1 = touch_read_adc(XPT2046_CHANNEL_X);
    y1 = touch_read_adc(XPT2046_CHANNEL_Y);
    x2 = touch_read_adc(XPT2046_CHANNEL_X);
    y2 = touch_read_adc(XPT2046_CHANNEL_Y);

    dx = (x1 > x2) ? (uint16_t)(x1 - x2) : (uint16_t)(x2 - x1);
    dy = (y1 > y2) ? (uint16_t)(y1 - y2) : (uint16_t)(y2 - y1);

    if (dx > 80U || dy > 80U) {
        return 0U;
    }

    *raw_x = (uint16_t)((x1 + x2) / 2U);
    *raw_y = (uint16_t)((y1 + y2) / 2U);
    return 1U;
}

static uint8_t touch_raw_to_screen(uint16_t raw_x, uint16_t raw_y, TouchPoint *point)
{
    int32_t sx;
    int32_t sy;

    /* Calibrated from measured presses on the visible 2x3 button pad.
       raw_y controls horizontal position, raw_x controls vertical position. */
    sx = 235L - ((int32_t)raw_y * 56L) / 1000L;
    sy = 304L - ((int32_t)raw_x * 17L) / 1000L;

    if (sx < 0 || sx >= (int32_t)LCD_WIDTH || sy < 0 || sy >= (int32_t)LCD_HEIGHT) {
        return 0U;
    }

    point->x = (int16_t)sx;
    point->y = (int16_t)sy;
    return 1U;
}

static TouchEvent touch_point_to_event(const TouchPoint *point)
{
    if (point->y >= TOUCH_BTN_BOT_Y0 && point->y < TOUCH_BTN_BOT_Y1) {
        if (point->x < TOUCH_BTN_COL0_X1) {
            return TOUCH_EVENT_START;
        }
        if (point->x < TOUCH_BTN_COL1_X1) {
            return TOUCH_EVENT_NONE;
        }
        if (point->x < TOUCH_BTN_COL2_X1) {
            return TOUCH_EVENT_MODE;
        }
    }

    return TOUCH_EVENT_NONE;
}

void touch_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN;

    gpio_set_cfg(TOUCH_CS_PORT, TOUCH_CS_PIN, 0x3U);
    gpio_set_cfg(TOUCH_CLK_PORT, TOUCH_CLK_PIN, 0x3U);
    gpio_set_cfg(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN, 0x3U);
    gpio_set_cfg(TOUCH_MISO_PORT, TOUCH_MISO_PIN, 0x8U);
    gpio_set_cfg(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN, 0x8U);

    TOUCH_MISO_PORT->ODR |= 1UL << TOUCH_MISO_PIN;
    TOUCH_IRQ_PORT->ODR |= 1UL << TOUCH_IRQ_PIN;

    pin_high(TOUCH_CS_PORT, TOUCH_CS_PIN);
    pin_low(TOUCH_CLK_PORT, TOUCH_CLK_PIN);
    pin_low(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN);
}

void touch_set_playing(uint8_t playing)
{
    g_is_playing = playing ? 1U : 0U;
}

void touch_reset_state(void)
{
    uint32_t now = millis();
    g_touch_active = 0U;
    g_invalid_active = 0U;
    g_start_raw_x = 0U;
    g_start_raw_y = 0U;
    g_last_raw_x = 0U;
    g_last_raw_y = 0U;
    g_last_touch_ms = now;
    g_last_sample_ms = now;
}

TouchEvent touch_scan_event(void)
{
    uint16_t raw_x;
    uint16_t raw_y;
    TouchPoint point;
    TouchEvent event;
    TouchEvent swipe_event;
    uint32_t now = millis();
    uint8_t pressed = touch_is_pressed();

    if (!pressed) {
        g_invalid_active = 0U;
        if (g_touch_active && g_is_playing) {
            swipe_event = touch_swipe_to_event(g_start_raw_x, g_start_raw_y, g_last_raw_x, g_last_raw_y);
            g_touch_active = 0U;
            if (swipe_event != TOUCH_EVENT_NONE) {
                g_last_touch_ms = now;
                touch_debug_swipe(swipe_event);
                return swipe_event;
            }
        }
        g_touch_active = 0U;
        return TOUCH_EVENT_NONE;
    }

    if ((now - g_last_touch_ms) < TOUCH_DEBOUNCE_MS ||
        (now - g_last_sample_ms) < TOUCH_SAMPLE_MS) {
        return TOUCH_EVENT_NONE;
    }
    g_last_sample_ms = now;

    if (!touch_read_raw(&raw_x, &raw_y)) {
        return TOUCH_EVENT_NONE;
    }

    if (g_is_playing) {
        if (!g_touch_active) {
            g_touch_active = 1U;
            g_start_raw_x = raw_x;
            g_start_raw_y = raw_y;
            g_last_raw_x = raw_x;
            g_last_raw_y = raw_y;
            touch_debug_point("swipe-start", raw_x, raw_y, 0);
            return TOUCH_EVENT_NONE;
        }

        g_last_raw_x = raw_x;
        g_last_raw_y = raw_y;
        return TOUCH_EVENT_NONE;
    }

    if (!touch_raw_to_screen(raw_x, raw_y, &point)) {
        if (!g_invalid_active) {
            g_invalid_active = 1U;
            touch_debug_point("out", raw_x, raw_y, 0);
        }
        return TOUCH_EVENT_NONE;
    }

    g_invalid_active = 0U;

    if (!g_touch_active) {
        g_touch_active = 1U;
        g_start_raw_x = raw_x;
        g_start_raw_y = raw_y;
        g_last_raw_x = raw_x;
        g_last_raw_y = raw_y;
        touch_debug_point("press", raw_x, raw_y, &point);
        event = touch_point_to_event(&point);
        if (event != TOUCH_EVENT_NONE) {
            g_last_touch_ms = now;
            touch_debug_event(event);
            return event;
        }
        touch_debug_miss(&point, raw_x, raw_y);
        return TOUCH_EVENT_NONE;
    }

    g_last_raw_x = raw_x;
    g_last_raw_y = raw_y;

    return TOUCH_EVENT_NONE;
}
