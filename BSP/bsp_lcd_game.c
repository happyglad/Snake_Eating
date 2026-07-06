#include "bsp_lcd_game.h"
#include "stm32f103_min.h"
#include "chinese_fonts.h"
#include "title_bitmap.h"
#include <string.h>

#define ILI9341_CMD_ADDR   (*(__IO uint16_t *)0x60000000UL)
#define ILI9341_DATA_ADDR  (*(__IO uint16_t *)0x60020000UL)
#define LCDID_UNKNOWN      0x0000U
#define LCDID_ILI9341      0x9341U
#define LCDID_ST7789V      0x8552U

static uint16_t g_lcd_id = LCDID_UNKNOWN;
static uint8_t g_scan_mode = 0U;
static uint16_t g_lcd_width = LCD_WIDTH;
static uint16_t g_lcd_height = LCD_HEIGHT;
static uint8_t g_vertical_flip = 0U;
static void (*g_lcd_trace_fn)(uint8_t stage) = 0;

static void gpio_set_cfg(GPIO_TypeDef *port, uint32_t pin, uint32_t cfg)
{
    volatile uint32_t *reg = (pin < 8U) ? &port->CRL : &port->CRH;
    uint32_t shift = (pin & 7U) * 4U;
    *reg = (*reg & ~(0xFUL << shift)) | (cfg << shift);
}

static void lcd_write_cmd(uint16_t cmd)
{
    ILI9341_CMD_ADDR = cmd;
}

static void lcd_write_data(uint16_t data)
{
    ILI9341_DATA_ADDR = data;
}

static uint16_t lcd_read_data(void)
{
    return ILI9341_DATA_ADDR;
}

static void lcd_backlight_control(uint8_t on)
{
    if (on) {
        GPIOD->BRR = 1UL << 12;
    } else {
        GPIOD->BSRR = 1UL << 12;
    }
}

void lcd_backlight_set(uint8_t on)
{
    lcd_backlight_control(on);
}

static void lcd_hw_reset(void)
{
    GPIOE->BRR = 1UL << 1;
    delay_ms(20);
    GPIOE->BSRR = 1UL << 1;
    delay_ms(20);
}

static void lcd_init_ili9341(void)
{
    lcd_write_cmd(0xCF);
    lcd_write_data(0x00);
    lcd_write_data(0x81);
    lcd_write_data(0x30);
    lcd_write_cmd(0xED);
    lcd_write_data(0x64);
    lcd_write_data(0x03);
    lcd_write_data(0x12);
    lcd_write_data(0x81);
    lcd_write_cmd(0xE8);
    lcd_write_data(0x85);
    lcd_write_data(0x10);
    lcd_write_data(0x78);
    lcd_write_cmd(0xCB);
    lcd_write_data(0x39);
    lcd_write_data(0x2C);
    lcd_write_data(0x00);
    lcd_write_data(0x34);
    lcd_write_data(0x06);
    lcd_write_cmd(0xF7);
    lcd_write_data(0x20);
    lcd_write_cmd(0xEA);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_cmd(0xB1);
    lcd_write_data(0x00);
    lcd_write_data(0x1B);
    lcd_write_cmd(0xB6);
    lcd_write_data(0x0A);
    lcd_write_data(0xA2);
    lcd_write_cmd(0xC0);
    lcd_write_data(0x35);
    lcd_write_cmd(0xC1);
    lcd_write_data(0x11);
    lcd_write_cmd(0xC5);
    lcd_write_data(0x45);
    lcd_write_data(0x45);
    lcd_write_cmd(0xC7);
    lcd_write_data(0xA2);
    lcd_write_cmd(0xF2);
    lcd_write_data(0x00);
    lcd_write_cmd(0x26);
    lcd_write_data(0x01);
    lcd_write_cmd(0xE0);
    lcd_write_data(0x0F);
    lcd_write_data(0x26);
    lcd_write_data(0x24);
    lcd_write_data(0x0B);
    lcd_write_data(0x0E);
    lcd_write_data(0x09);
    lcd_write_data(0x54);
    lcd_write_data(0xA8);
    lcd_write_data(0x46);
    lcd_write_data(0x0C);
    lcd_write_data(0x17);
    lcd_write_data(0x09);
    lcd_write_data(0x0F);
    lcd_write_data(0x07);
    lcd_write_data(0x00);
    lcd_write_cmd(0xE1);
    lcd_write_data(0x00);
    lcd_write_data(0x19);
    lcd_write_data(0x1B);
    lcd_write_data(0x04);
    lcd_write_data(0x10);
    lcd_write_data(0x07);
    lcd_write_data(0x2A);
    lcd_write_data(0x47);
    lcd_write_data(0x39);
    lcd_write_data(0x03);
    lcd_write_data(0x06);
    lcd_write_data(0x06);
    lcd_write_data(0x30);
    lcd_write_data(0x38);
    lcd_write_data(0x0F);
    lcd_write_cmd(0x36);
    lcd_write_data(0xC8);
    lcd_write_cmd(0x2A);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0xEF);
    lcd_write_cmd(0x2B);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x01);
    lcd_write_data(0x3F);
    lcd_write_cmd(0x3A);
    lcd_write_data(0x55);
    lcd_write_cmd(0x11);
    delay_ms(120);
    lcd_write_cmd(0x29);
}

static void lcd_init_st7789v(void)
{
    lcd_write_cmd(0xCF);
    lcd_write_data(0x00);
    lcd_write_data(0xC1);
    lcd_write_data(0x30);
    lcd_write_cmd(0xED);
    lcd_write_data(0x64);
    lcd_write_data(0x03);
    lcd_write_data(0x12);
    lcd_write_data(0x81);
    lcd_write_cmd(0xE8);
    lcd_write_data(0x85);
    lcd_write_data(0x10);
    lcd_write_data(0x78);
    lcd_write_cmd(0xCB);
    lcd_write_data(0x39);
    lcd_write_data(0x2C);
    lcd_write_data(0x00);
    lcd_write_data(0x34);
    lcd_write_data(0x02);
    lcd_write_cmd(0xF7);
    lcd_write_data(0x20);
    lcd_write_cmd(0xEA);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_cmd(0xC0);
    lcd_write_data(0x21);
    lcd_write_cmd(0xC1);
    lcd_write_data(0x11);
    lcd_write_cmd(0xC5);
    lcd_write_data(0x2D);
    lcd_write_data(0x33);
    lcd_write_cmd(0x36);
    lcd_write_data(0x00);
    lcd_write_cmd(0x3A);
    lcd_write_data(0x55);
    lcd_write_cmd(0xF2);
    lcd_write_data(0x00);
    lcd_write_cmd(0x26);
    lcd_write_data(0x01);
    lcd_write_cmd(0xE0);
    lcd_write_data(0xD0);
    lcd_write_data(0x00);
    lcd_write_data(0x02);
    lcd_write_data(0x07);
    lcd_write_data(0x0B);
    lcd_write_data(0x1A);
    lcd_write_data(0x31);
    lcd_write_data(0x54);
    lcd_write_data(0x40);
    lcd_write_data(0x29);
    lcd_write_data(0x12);
    lcd_write_data(0x12);
    lcd_write_data(0x12);
    lcd_write_data(0x17);
    lcd_write_cmd(0xE1);
    lcd_write_data(0xD0);
    lcd_write_data(0x00);
    lcd_write_data(0x02);
    lcd_write_data(0x07);
    lcd_write_data(0x05);
    lcd_write_data(0x25);
    lcd_write_data(0x2D);
    lcd_write_data(0x44);
    lcd_write_data(0x45);
    lcd_write_data(0x1C);
    lcd_write_data(0x18);
    lcd_write_data(0x16);
    lcd_write_data(0x1C);
    lcd_write_data(0x1D);
    lcd_write_cmd(0x11);
    delay_ms(120);
    lcd_write_cmd(0x29);
}

static void lcd_gram_scan(uint8_t mode)
{
    if (mode > 7U) {
        return;
    }

    g_scan_mode = mode;
    if ((mode & 1U) == 0U) {
        g_lcd_width = 240U;
        g_lcd_height = 320U;
    } else {
        g_lcd_width = 320U;
        g_lcd_height = 240U;
    }

    lcd_write_cmd(0x36);
    if (g_lcd_id == LCDID_ST7789V) {
        uint16_t madctl = (uint16_t)((mode << 5) & 0xE0U);
        if (g_vertical_flip) {
            madctl ^= 0x80U;
        }
        lcd_write_data(madctl);
    } else {
        uint16_t madctl = (uint16_t)(0x08U | ((mode << 5) & 0xE0U));
        if (g_vertical_flip) {
            madctl ^= 0x80U;
        }
        lcd_write_data(madctl);
    }

    lcd_write_cmd(0x2A);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data((uint16_t)((g_lcd_width - 1U) >> 8));
    lcd_write_data((uint16_t)((g_lcd_width - 1U) & 0xFFU));

    lcd_write_cmd(0x2B);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data((uint16_t)((g_lcd_height - 1U) >> 8));
    lcd_write_data((uint16_t)((g_lcd_height - 1U) & 0xFFU));

    lcd_write_cmd(0x2C);
}

static uint16_t lcd_read_id(void)
{
    uint16_t id;

    lcd_write_cmd(0x04);
    (void)lcd_read_data();
    (void)lcd_read_data();
    id = (uint16_t)(lcd_read_data() << 8);
    id |= lcd_read_data();
    if (id == LCDID_ST7789V) {
        return id;
    }

    lcd_write_cmd(0xD3);
    (void)lcd_read_data();
    (void)lcd_read_data();
    id = (uint16_t)(lcd_read_data() << 8);
    id |= lcd_read_data();
    if (id == LCDID_ILI9341) {
        return id;
    }

    return LCDID_UNKNOWN;
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    lcd_write_cmd(0x2A);
    lcd_write_data(x0 >> 8);
    lcd_write_data(x0 & 0xFFU);
    lcd_write_data(x1 >> 8);
    lcd_write_data(x1 & 0xFFU);

    lcd_write_cmd(0x2B);
    lcd_write_data(y0 >> 8);
    lcd_write_data(y0 & 0xFFU);
    lcd_write_data(y1 >> 8);
    lcd_write_data(y1 & 0xFFU);

    lcd_write_cmd(0x2C);
}

static void lcd_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN | RCC_APB2ENR_IOPEEN;
    RCC->AHBENR |= RCC_AHBENR_FSMCEN;

    gpio_set_cfg(GPIOD, 0, 0xB);  /* D2 */
    gpio_set_cfg(GPIOD, 1, 0xB);  /* D3 */
    gpio_set_cfg(GPIOD, 4, 0xB);  /* NOE/RD */
    gpio_set_cfg(GPIOD, 5, 0xB);  /* NWE/WR */
    gpio_set_cfg(GPIOD, 7, 0xB);  /* NE1/CS */
    gpio_set_cfg(GPIOD, 8, 0xB);  /* D13 */
    gpio_set_cfg(GPIOD, 9, 0xB);  /* D14 */
    gpio_set_cfg(GPIOD, 10, 0xB); /* D15 */
    gpio_set_cfg(GPIOD, 11, 0xB); /* A16/DC */
    gpio_set_cfg(GPIOD, 14, 0xB); /* D0 */
    gpio_set_cfg(GPIOD, 15, 0xB); /* D1 */

    gpio_set_cfg(GPIOE, 7, 0xB);
    gpio_set_cfg(GPIOE, 8, 0xB);
    gpio_set_cfg(GPIOE, 9, 0xB);
    gpio_set_cfg(GPIOE, 10, 0xB);
    gpio_set_cfg(GPIOE, 11, 0xB);
    gpio_set_cfg(GPIOE, 12, 0xB);
    gpio_set_cfg(GPIOE, 13, 0xB);
    gpio_set_cfg(GPIOE, 14, 0xB);
    gpio_set_cfg(GPIOE, 15, 0xB);

    gpio_set_cfg(GPIOE, 1, 0x3);  /* LCD reset */
    gpio_set_cfg(GPIOD, 12, 0x3); /* LCD backlight */
}

static void lcd_fsmc_init(void)
{
    FSMC_Bank1->BTCR[0] = 0x00001059UL;
    FSMC_Bank1->BTCR[1] = 0x10000401UL;
}

static void lcd_controller_init(void)
{
    delay_ms(50);
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(0);
    }
    g_lcd_id = lcd_read_id();
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(1);
    }
    if (g_lcd_id == LCDID_ST7789V) {
        lcd_init_st7789v();
    } else {
        g_lcd_id = LCDID_ILI9341; /* Fallback to ILI9341 if unknown */
        lcd_init_ili9341();
    }
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(2);
    }

    lcd_gram_scan(g_scan_mode);
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(3);
    }
}

void lcd_game_init(void)
{
    g_lcd_trace_fn = 0;
    lcd_game_init_trace(0);
}

void lcd_game_init_trace(void (*trace_fn)(uint8_t stage))
{
    g_lcd_trace_fn = trace_fn;
    lcd_gpio_init();
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(4);
    }
    lcd_fsmc_init();
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(5);
    }
    lcd_backlight_control(0);
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(6);
    }
    lcd_hw_reset();
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(7);
    }
    lcd_controller_init();
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(8);
    }
    lcd_clear(COLOR_BLACK);
    lcd_backlight_control(1);
    if (g_lcd_trace_fn) {
        g_lcd_trace_fn(9);
    }
}

void lcd_clear(uint16_t color)
{
    lcd_fill_rect(0, 0, g_lcd_width, g_lcd_height, color);
}

void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint32_t count;
    if (x >= g_lcd_width || y >= g_lcd_height || w == 0U || h == 0U) {
        return;
    }
    if ((x + w) > g_lcd_width) {
        w = g_lcd_width - x;
    }
    if ((y + h) > g_lcd_height) {
        h = g_lcd_height - y;
    }
    count = (uint32_t)w * (uint32_t)h;
    lcd_set_window(x, y, x + w - 1U, y + h - 1U);
    
    volatile uint16_t *data_reg = (__IO uint16_t *)0x60020000UL;
    while (count--) {
        *data_reg = color;
    }
}

static const uint8_t *glyph_for(char ch)
{
    static const uint8_t space[5] = {0,0,0,0,0};
    static const uint8_t colon[5] = {0,0x36,0x36,0,0};
    static const uint8_t zero[5] = {0x3E,0x51,0x49,0x45,0x3E};
    static const uint8_t one[5] = {0x00,0x42,0x7F,0x40,0x00};
    static const uint8_t two[5] = {0x42,0x61,0x51,0x49,0x46};
    static const uint8_t three[5] = {0x21,0x41,0x45,0x4B,0x31};
    static const uint8_t four[5] = {0x18,0x14,0x12,0x7F,0x10};
    static const uint8_t five[5] = {0x27,0x45,0x45,0x45,0x39};
    static const uint8_t six[5] = {0x3C,0x4A,0x49,0x49,0x30};
    static const uint8_t seven[5] = {0x01,0x71,0x09,0x05,0x03};
    static const uint8_t eight[5] = {0x36,0x49,0x49,0x49,0x36};
    static const uint8_t nine[5] = {0x06,0x49,0x49,0x29,0x1E};
    static const uint8_t A[5] = {0x7E,0x11,0x11,0x11,0x7E};
    static const uint8_t B[5] = {0x7F,0x49,0x49,0x49,0x36};
    static const uint8_t C[5] = {0x3E,0x41,0x41,0x41,0x22};
    static const uint8_t D[5] = {0x7F,0x41,0x41,0x22,0x1C};
    static const uint8_t E[5] = {0x7F,0x49,0x49,0x49,0x41};
    static const uint8_t G[5] = {0x3E,0x41,0x49,0x49,0x7A};
    static const uint8_t H[5] = {0x7F,0x08,0x08,0x08,0x7F};
    static const uint8_t I[5] = {0x00,0x41,0x7F,0x41,0x00};
    static const uint8_t K[5] = {0x7F,0x08,0x14,0x22,0x41};
    static const uint8_t L[5] = {0x7F,0x40,0x40,0x40,0x40};
    static const uint8_t M[5] = {0x7F,0x02,0x0C,0x02,0x7F};
    static const uint8_t N[5] = {0x7F,0x04,0x08,0x10,0x7F};
    static const uint8_t O[5] = {0x3E,0x41,0x41,0x41,0x3E};
    static const uint8_t P[5] = {0x7F,0x09,0x09,0x09,0x06};
    static const uint8_t R[5] = {0x7F,0x09,0x19,0x29,0x46};
    static const uint8_t S[5] = {0x46,0x49,0x49,0x49,0x31};
    static const uint8_t T[5] = {0x01,0x01,0x7F,0x01,0x01};
    static const uint8_t U[5] = {0x3F,0x40,0x40,0x40,0x3F};
    static const uint8_t V[5] = {0x1F,0x20,0x40,0x20,0x1F};
    static const uint8_t W[5] = {0x7F,0x20,0x18,0x20,0x7F};
    static const uint8_t Y[5] = {0x07,0x08,0x70,0x08,0x07};

    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - 'a' + 'A');
    }
    switch (ch) {
    case '0': return zero; case '1': return one; case '2': return two;
    case '3': return three; case '4': return four; case '5': return five;
    case '6': return six; case '7': return seven; case '8': return eight;
    case '9': return nine; case ':': return colon; case ' ': return space;
    case 'A': return A; case 'B': return B; case 'C': return C; case 'D': return D; case 'E': return E; case 'G': return G;
    case 'H': return H; case 'I': return I; case 'L': return L; case 'M': return M; case 'N': return N;
    case 'K': return K; case 'O': return O; case 'P': return P; case 'R': return R; case 'S': return S;
    case 'T': return T; case 'U': return U; case 'V': return V; case 'W': return W; case 'Y': return Y; default: return space;
    }
}

static void lcd_draw_char(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg)
{
    const uint8_t *g = glyph_for(ch);
    uint16_t dy;
    uint16_t dx;
    
    /* Set a single window of 12x14 pixels for the entire character bounding box */
    lcd_set_window(x, y, x + 11U, y + 13U);
    
    for (dy = 0; dy < 14U; dy++) {
        uint8_t row = (uint8_t)(dy >> 1U); /* dy / 2 */
        uint8_t row_bit = 1U << row;
        
        for (dx = 0; dx < 12U; dx++) {
            uint8_t col = (uint8_t)(dx >> 1U); /* dx / 2 */
            
            if (col < 5U) {
                if (g[col] & row_bit) {
                    lcd_write_data(color);
                } else {
                    lcd_write_data(bg);
                }
            } else {
                lcd_write_data(bg);
            }
        }
    }
}

void lcd_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg)
{
    while (*text) {
        lcd_draw_char(x, y, *text++, color, bg);
        x += 12U;
    }
}

void lcd_draw_number(uint16_t x, uint16_t y, uint32_t number, uint16_t color, uint16_t bg)
{
    char buf[11];
    uint8_t i = 0;
    uint8_t j;
    if (number == 0U) {
        lcd_draw_text(x, y, "0", color, bg);
        return;
    }
    while (number && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (number % 10U));
        number /= 10U;
    }
    for (j = 0; j < i; j++) {
        lcd_draw_char((uint16_t)(x + j * 12U), y, buf[i - 1U - j], color, bg);
    }
}

void lcd_draw_cell(uint8_t x, uint8_t y, uint16_t color)
{
    lcd_fill_rect((uint16_t)(BOARD_X + x * CELL_SIZE + 1U),
                  (uint16_t)(BOARD_Y + y * CELL_SIZE + 1U),
                  CELL_SIZE - 2U,
                  CELL_SIZE - 2U,
                  color);
}

void lcd_draw_snake_head(uint8_t x, uint8_t y, uint8_t dir)
{
    uint16_t px = (uint16_t)(BOARD_X + x * CELL_SIZE + 1U);
    uint16_t py = (uint16_t)(BOARD_Y + y * CELL_SIZE + 1U);
    uint16_t eye1_x = (uint16_t)(px + 3U);
    uint16_t eye1_y = (uint16_t)(py + 3U);
    uint16_t eye2_x = (uint16_t)(px + 7U);
    uint16_t eye2_y = (uint16_t)(py + 3U);

    lcd_fill_rect(px, py, CELL_SIZE - 2U, CELL_SIZE - 2U, COLOR_GREEN);
    if (dir == 0U) {
        eye1_x = (uint16_t)(px + 3U); eye1_y = (uint16_t)(py + 2U);
        eye2_x = (uint16_t)(px + 7U); eye2_y = (uint16_t)(py + 2U);
    } else if (dir == 1U) {
        eye1_x = (uint16_t)(px + 3U); eye1_y = (uint16_t)(py + 7U);
        eye2_x = (uint16_t)(px + 7U); eye2_y = (uint16_t)(py + 7U);
    } else if (dir == 2U) {
        eye1_x = (uint16_t)(px + 2U); eye1_y = (uint16_t)(py + 3U);
        eye2_x = (uint16_t)(px + 2U); eye2_y = (uint16_t)(py + 7U);
    } else {
        eye1_x = (uint16_t)(px + 7U); eye1_y = (uint16_t)(py + 3U);
        eye2_x = (uint16_t)(px + 7U); eye2_y = (uint16_t)(py + 7U);
    }
    lcd_fill_rect(eye1_x, eye1_y, 2U, 2U, COLOR_BLACK);
    lcd_fill_rect(eye2_x, eye2_y, 2U, 2U, COLOR_BLACK);
}

void lcd_draw_snake_body(uint8_t x, uint8_t y)
{
    uint16_t px = (uint16_t)(BOARD_X + x * CELL_SIZE + 1U);
    uint16_t py = (uint16_t)(BOARD_Y + y * CELL_SIZE + 1U);
    lcd_fill_rect(px, py, CELL_SIZE - 2U, CELL_SIZE - 2U, COLOR_CYAN);
    lcd_fill_rect((uint16_t)(px + 2U), (uint16_t)(py + 2U), CELL_SIZE - 6U, CELL_SIZE - 6U, COLOR_LIME);
}

void lcd_draw_food(uint8_t x, uint8_t y)
{
    uint16_t px = (uint16_t)(BOARD_X + x * CELL_SIZE + 1U);
    uint16_t py = (uint16_t)(BOARD_Y + y * CELL_SIZE + 1U);
    lcd_fill_rect(px, py, CELL_SIZE - 2U, CELL_SIZE - 2U, COLOR_BLACK);
    lcd_fill_rect((uint16_t)(px + 3U), (uint16_t)(py + 2U), 5U, 7U, COLOR_RED);
    lcd_fill_rect((uint16_t)(px + 5U), py, 2U, 2U, COLOR_GREEN);
    lcd_fill_rect((uint16_t)(px + 2U), (uint16_t)(py + 4U), 7U, 3U, COLOR_ORANGE);
}

void lcd_draw_obstacle(uint8_t x, uint8_t y)
{
    uint16_t px = (uint16_t)(BOARD_X + x * CELL_SIZE + 1U);
    uint16_t py = (uint16_t)(BOARD_Y + y * CELL_SIZE + 1U);
    lcd_fill_rect(px, py, CELL_SIZE - 2U, CELL_SIZE - 2U, COLOR_GRAY);
    lcd_fill_rect((uint16_t)(px + 1U), (uint16_t)(py + 1U), CELL_SIZE - 4U, 2U, COLOR_WHITE);
    lcd_fill_rect((uint16_t)(px + 2U), (uint16_t)(py + 5U), CELL_SIZE - 6U, 2U, COLOR_DARK);
}

void lcd_draw_touch_controls(void)
{
    lcd_fill_rect(0, 224, g_lcd_width, 96, COLOR_BLACK);
    
    /* MODE button */
    lcd_fill_rect(0, 272, 80, 48, COLOR_GRAY);
    lcd_fill_rect(2, 274, 76, 44, COLOR_DARK);
    lcd_draw_chinese_text(24, 288, "切换", COLOR_ORANGE, COLOR_DARK);
    
    /* START button */
    lcd_fill_rect(160, 272, 80, 48, COLOR_GRAY);
    lcd_fill_rect(162, 274, 76, 44, COLOR_DARK);
    lcd_draw_chinese_text(184, 288, "开始", COLOR_YELLOW, COLOR_DARK);
}

void lcd_draw_swipe_controls(void)
{
    lcd_fill_rect(0, 224, g_lcd_width, 96, COLOR_BLACK);
    lcd_fill_rect(0, 224, g_lcd_width, 96, COLOR_DARK);
    lcd_fill_rect(2, 226, (uint16_t)(g_lcd_width - 4U), 92, COLOR_BLACK);
}

void lcd_draw_board(uint32_t score, uint32_t high_score)
{
    lcd_draw_board_ex(score, high_score, "NORMAL");
}

void lcd_draw_board_ex(uint32_t score, uint32_t high_score, const char *mode)
{
    const char *zh_mode = "普通";
    
    lcd_fill_rect(0, 0, g_lcd_width, 40, COLOR_BLACK);
    lcd_fill_rect(0, 0, g_lcd_width, 34, COLOR_DARK);
    lcd_draw_text(4, 4, "S:", COLOR_WHITE, COLOR_DARK);
    lcd_draw_number(28, 4, score, COLOR_YELLOW, COLOR_DARK);
    lcd_draw_text(84, 4, "H:", COLOR_WHITE, COLOR_DARK);
    lcd_draw_number(108, 4, high_score, COLOR_CYAN, COLOR_DARK);
    lcd_draw_text(4, 22, "M:", COLOR_WHITE, COLOR_DARK);
    
    if (strcmp(mode, "BLOCK") == 0) {
        zh_mode = "障碍";
    } else if (strcmp(mode, "TIME") == 0) {
        zh_mode = "限时";
    }
    lcd_draw_chinese_text(28, 19, zh_mode, COLOR_ORANGE, COLOR_DARK);
    
    lcd_draw_text(144, 22, "T:", COLOR_WHITE, COLOR_DARK);
    lcd_draw_number(168, 22, 0U, COLOR_GREEN, COLOR_DARK);
    
    /* Clear active item status slot */
    lcd_fill_rect(164, 4, 76, 16, COLOR_DARK);

    lcd_fill_rect(BOARD_X, BOARD_Y,
                  GRID_COLS * CELL_SIZE,
                  GRID_ROWS * CELL_SIZE, COLOR_BLACK);
    lcd_fill_rect(BOARD_X, BOARD_Y,
                  GRID_COLS * CELL_SIZE,
                  GRID_ROWS * CELL_SIZE, COLOR_WHITE);
    lcd_fill_rect(BOARD_X + 1U, BOARD_Y + 1U,
                  GRID_COLS * CELL_SIZE - 2U,
                  GRID_ROWS * CELL_SIZE - 2U, COLOR_BLACK);
}

void lcd_update_score(uint32_t score, uint32_t high_score)
{
    lcd_update_status(score, high_score, 0U);
}

void lcd_update_status(uint32_t score, uint32_t high_score, uint32_t duration)
{
    lcd_update_status_ex(score, high_score, duration, "", 0U);
}

void lcd_update_status_ex(uint32_t score, uint32_t high_score, uint32_t duration, const char *item_text, uint32_t item_sec)
{
    lcd_fill_rect(28, 4, 54, 14, COLOR_DARK);
    lcd_draw_number(28, 4, score, COLOR_YELLOW, COLOR_DARK);
    lcd_fill_rect(108, 4, 54, 14, COLOR_DARK);
    lcd_draw_number(108, 4, high_score, COLOR_CYAN, COLOR_DARK);
    lcd_fill_rect(168, 22, 48, 14, COLOR_DARK);
    lcd_draw_number(168, 22, duration, COLOR_GREEN, COLOR_DARK);
    
    /* Draw item status text at x=164, y=4 */
    lcd_fill_rect(164, 4, 76, 16, COLOR_DARK);
    if (item_text && item_text[0] != '\0') {
        lcd_draw_chinese_text(164, 4, item_text, COLOR_LIME, COLOR_DARK);
        lcd_draw_text(196, 5, ":", COLOR_LIME, COLOR_DARK);
        lcd_draw_number(208, 5, item_sec, COLOR_LIME, COLOR_DARK);
    }
}

void lcd_show_start(uint32_t high_score)
{
    lcd_show_start_ex(high_score, "NORMAL");
}

void lcd_show_start_ex(uint32_t high_score, const char *mode)
{
    uint16_t mode_y = 88U;
    
    lcd_clear(COLOR_BLACK);
    
    /* Top decorative header bar */
    lcd_fill_rect(0, 0, g_lcd_width, 60, COLOR_DARK);
    lcd_fill_rect(0, 58, g_lcd_width, 2, COLOR_GREEN);
    
    /* Chinese Title: "贪吃蛇大冒险" in custom 32x32 snake-themed pixel art */
    lcd_draw_snake_title_32(10, 3, COLOR_GREEN, COLOR_DARK);
    
    /* Mode selection title */
    lcd_draw_chinese_text(36, 68, "请选择游戏模式:", COLOR_WHITE, COLOR_BLACK);
    
    /* Determine y coordinate for highlighting current mode */
    if (strcmp(mode, "BLOCK") == 0) {
        mode_y = 113U;
    } else if (strcmp(mode, "TIME") == 0) {
        mode_y = 138U;
    } else {
        mode_y = 88U;
    }
    
    /* Draw highlight box for active mode selection */
    lcd_fill_rect(24, mode_y - 2, 192, 20, COLOR_DARK);
    
    /* Render mode strings */
    lcd_draw_chinese_text(36, 90, "普通模式", (mode_y == 88U) ? COLOR_YELLOW : COLOR_GRAY, (mode_y == 88U) ? COLOR_DARK : COLOR_BLACK);
    lcd_draw_chinese_text(36, 115, "障碍模式", (mode_y == 113U) ? COLOR_YELLOW : COLOR_GRAY, (mode_y == 113U) ? COLOR_DARK : COLOR_BLACK);
    lcd_draw_chinese_text(36, 140, "限时挑战", (mode_y == 138U) ? COLOR_YELLOW : COLOR_GRAY, (mode_y == 138U) ? COLOR_DARK : COLOR_BLACK);
    
    /* Render high score stats */
    lcd_draw_chinese_text(36, 168, "最高记录:", COLOR_CYAN, COLOR_BLACK);
    lcd_draw_number(116, 168, high_score, COLOR_YELLOW, COLOR_BLACK);
    
    /* Render control tip */
    lcd_draw_chinese_text(36, 196, "按按键开始游戏", COLOR_WHITE, COLOR_BLACK);
    
    lcd_draw_touch_controls();
}

void lcd_show_start_revamp(uint8_t focus, uint32_t high_score)
{
    lcd_clear(COLOR_BLACK);
    
    /* Top decorative header bar */
    lcd_fill_rect(0, 0, g_lcd_width, 60, COLOR_DARK);
    lcd_fill_rect(0, 58, g_lcd_width, 2, COLOR_GREEN);
    
    /* Chinese Title: "贪吃蛇大冒险" in custom 32x32 snake-themed pixel art */
    lcd_draw_snake_title_32(10, 3, COLOR_GREEN, COLOR_DARK);
    
    /* Option 1: 开始游戏 */
    lcd_fill_rect(32, 85, 176, 36, (focus == 0U) ? COLOR_DARK : COLOR_BLACK);
    if (focus == 0U) {
        /* Draw blue frame for focus */
        lcd_fill_rect(32, 85, 176, 2, COLOR_CYAN);
        lcd_fill_rect(32, 119, 176, 2, COLOR_CYAN);
        lcd_fill_rect(32, 85, 2, 36, COLOR_CYAN);
        lcd_fill_rect(206, 85, 2, 36, COLOR_CYAN);
    }
    lcd_draw_chinese_text(72, 95, "开始游戏", (focus == 0U) ? COLOR_YELLOW : COLOR_GRAY, (focus == 0U) ? COLOR_DARK : COLOR_BLACK);
    
    /* Option 2: 游戏设置 */
    lcd_fill_rect(32, 135, 176, 36, (focus == 1U) ? COLOR_DARK : COLOR_BLACK);
    if (focus == 1U) {
        /* Draw blue frame for focus */
        lcd_fill_rect(32, 135, 176, 2, COLOR_CYAN);
        lcd_fill_rect(32, 169, 176, 2, COLOR_CYAN);
        lcd_fill_rect(32, 135, 2, 36, COLOR_CYAN);
        lcd_fill_rect(206, 135, 2, 36, COLOR_CYAN);
    }
    lcd_draw_chinese_text(72, 145, "游戏设置", (focus == 1U) ? COLOR_YELLOW : COLOR_GRAY, (focus == 1U) ? COLOR_DARK : COLOR_BLACK);
    
    /* Render high score stats */
    lcd_draw_chinese_text(36, 185, "最高记录:", COLOR_CYAN, COLOR_BLACK);
    lcd_draw_number(116, 185, high_score, COLOR_YELLOW, COLOR_BLACK);
    
    /* Draw revamped touch controls at bottom */
    lcd_fill_rect(0, 224, g_lcd_width, 96, COLOR_BLACK);
    
    /* Left button: "切换" */
    lcd_fill_rect(0, 272, 80, 48, COLOR_GRAY);
    lcd_fill_rect(2, 274, 76, 44, COLOR_DARK);
    lcd_draw_chinese_text(8, 288, "切换", COLOR_WHITE, COLOR_DARK);
    
    /* Right button: "确定" */
    lcd_fill_rect(160, 272, 80, 48, COLOR_GRAY);
    lcd_fill_rect(162, 274, 76, 44, COLOR_DARK);
    lcd_draw_chinese_text(168, 288, "确定", COLOR_WHITE, COLOR_DARK);
}

void lcd_show_settings(uint8_t focus, const char *mode, uint8_t items_enabled)
{
    lcd_clear(COLOR_BLACK);
    
    /* Top decorative header bar */
    lcd_fill_rect(0, 0, g_lcd_width, 46, COLOR_DARK);
    lcd_fill_rect(0, 44, g_lcd_width, 2, COLOR_GREEN);
    
    /* Title: "游戏设置" (32x32 size) */
    lcd_draw_chinese_text_32(56, 7, "游戏设置", COLOR_GREEN, COLOR_DARK);
    
    /* Row 0: 模式选择 */
    lcd_fill_rect(16, 68, 208, 36, (focus == 0U) ? COLOR_DARK : COLOR_BLACK);
    if (focus == 0U) {
        lcd_fill_rect(16, 68, 208, 2, COLOR_CYAN);
        lcd_fill_rect(16, 102, 208, 2, COLOR_CYAN);
        lcd_fill_rect(16, 68, 2, 36, COLOR_CYAN);
        lcd_fill_rect(222, 68, 2, 36, COLOR_CYAN);
    }
    lcd_draw_chinese_text(24, 78, "模式选择:", COLOR_WHITE, (focus == 0U) ? COLOR_DARK : COLOR_BLACK);
    
    /* Get mode Chinese name */
    const char *zh_mode = "普通";
    if (strcmp(mode, "BLOCKS") == 0 || strcmp(mode, "BLOCK") == 0) {
        zh_mode = "障碍";
    } else if (strcmp(mode, "TIME_LIMIT") == 0 || strcmp(mode, "TIME") == 0) {
        zh_mode = "限时";
    }
    lcd_draw_chinese_text(112, 78, zh_mode, COLOR_YELLOW, (focus == 0U) ? COLOR_DARK : COLOR_BLACK);
    
    /* Row 1: 道具开启 */
    lcd_fill_rect(16, 114, 208, 36, (focus == 1U) ? COLOR_DARK : COLOR_BLACK);
    if (focus == 1U) {
        lcd_fill_rect(16, 114, 208, 2, COLOR_CYAN);
        lcd_fill_rect(16, 148, 208, 2, COLOR_CYAN);
        lcd_fill_rect(16, 114, 2, 36, COLOR_CYAN);
        lcd_fill_rect(222, 114, 2, 36, COLOR_CYAN);
    }
    lcd_draw_chinese_text(24, 124, "道具开启:", COLOR_WHITE, (focus == 1U) ? COLOR_DARK : COLOR_BLACK);
    lcd_draw_chinese_text(112, 124, items_enabled ? "开启" : "关闭", items_enabled ? COLOR_GREEN : COLOR_RED, (focus == 1U) ? COLOR_DARK : COLOR_BLACK);
    
    /* Row 2: 返回主菜单 */
    lcd_fill_rect(16, 160, 208, 36, (focus == 2U) ? COLOR_DARK : COLOR_BLACK);
    if (focus == 2U) {
        lcd_fill_rect(16, 160, 208, 2, COLOR_CYAN);
        lcd_fill_rect(16, 194, 208, 2, COLOR_CYAN);
        lcd_fill_rect(16, 160, 2, 36, COLOR_CYAN);
        lcd_fill_rect(222, 160, 2, 36, COLOR_CYAN);
    }
    lcd_draw_chinese_text(72, 170, "返回主菜单", (focus == 2U) ? COLOR_YELLOW : COLOR_GRAY, (focus == 2U) ? COLOR_DARK : COLOR_BLACK);
    
    /* Touch controls at bottom */
    lcd_fill_rect(0, 224, g_lcd_width, 96, COLOR_BLACK);
    
    /* Left button: "切换" */
    lcd_fill_rect(0, 272, 80, 48, COLOR_GRAY);
    lcd_fill_rect(2, 274, 76, 44, COLOR_DARK);
    lcd_draw_chinese_text(8, 288, "切换", COLOR_WHITE, COLOR_DARK);
    
    /* Right button: "修改" / "返回" */
    lcd_fill_rect(160, 272, 80, 48, COLOR_GRAY);
    lcd_fill_rect(162, 274, 76, 44, COLOR_DARK);
    lcd_draw_chinese_text(168, 288, (focus == 2U) ? "返回" : "修改", COLOR_WHITE, COLOR_DARK);
}

void lcd_show_game_over(uint32_t score, uint32_t high_score)
{
    lcd_show_game_over_ex(score, high_score, 0U, "HIT");
}

void lcd_show_game_over_ex(uint32_t score, uint32_t high_score, uint32_t duration, const char *reason)
{
    const char *zh_reason = "结束";
    
    lcd_fill_rect(16, 82, 208, 142, COLOR_BLACK);
    lcd_fill_rect(20, 86, 200, 134, COLOR_DARK);
    
    lcd_draw_chinese_text(88, 98, "游戏结束", COLOR_RED, COLOR_DARK);
    lcd_draw_chinese_text(38, 126, "原因:", COLOR_WHITE, COLOR_DARK);
    
    if (strcmp(reason, "WALL") == 0) {
        zh_reason = "撞墙死亡";
    } else if (strcmp(reason, "BODY") == 0) {
        zh_reason = "撞到身体";
    } else if (strcmp(reason, "BLOCK") == 0) {
        zh_reason = "撞到障碍";
    } else if (strcmp(reason, "TIMEOUT") == 0) {
        zh_reason = "时间结束";
    }
    
    lcd_draw_chinese_text(86, 126, zh_reason, COLOR_ORANGE, COLOR_DARK);
    
    lcd_draw_chinese_text(38, 150, "得分:", COLOR_WHITE, COLOR_DARK);
    lcd_draw_number(86, 150, score, COLOR_YELLOW, COLOR_DARK);
    
    lcd_draw_chinese_text(38, 174, "最高:", COLOR_CYAN, COLOR_DARK);
    lcd_draw_number(86, 174, high_score, COLOR_YELLOW, COLOR_DARK);
    
    lcd_draw_chinese_text(38, 198, "时间:", COLOR_GREEN, COLOR_DARK);
    lcd_draw_number(86, 198, duration, COLOR_YELLOW, COLOR_DARK);
    lcd_draw_chinese_text(134, 198, "秒", COLOR_GREEN, COLOR_DARK);
    
    lcd_draw_touch_controls();
}

/* Chinese character rendering support */

void lcd_draw_chinese_char_16(uint16_t x, uint16_t y, const uint8_t *matrix, uint16_t color, uint16_t bg)
{
    uint16_t i, j;
    lcd_set_window(x, y, x + 15U, y + 15U);
    for (i = 0; i < 16U; i++) {
        uint16_t row = (uint16_t)(((uint16_t)matrix[2U * i] << 8) | matrix[2U * i + 1U]);
        for (j = 0; j < 16U; j++) {
            if (row & (0x8000U >> j)) {
                lcd_write_data(color);
            } else {
                lcd_write_data(bg);
            }
        }
    }
}

void lcd_draw_chinese_char_32(uint16_t x, uint16_t y, const uint8_t *matrix, uint16_t color, uint16_t bg)
{
    uint16_t i, j, r_scale;
    lcd_set_window(x, y, x + 31U, y + 31U);
    for (i = 0; i < 16U; i++) {
        uint16_t row = (uint16_t)(((uint16_t)matrix[2U * i] << 8) | matrix[2U * i + 1U]);
        for (r_scale = 0; r_scale < 2U; r_scale++) {
            for (j = 0; j < 16U; j++) {
                uint16_t pixel = (row & (0x8000U >> j)) ? color : bg;
                lcd_write_data(pixel);
                lcd_write_data(pixel);
            }
        }
    }
}

static const uint8_t *find_chinese_glyph(const uint8_t *code, uint8_t *bytes_consumed)
{
    uint16_t i;
    if (code[0] >= 0xE0U && code[0] <= 0xEFU) {
        *bytes_consumed = 3U;
        for (i = 0; i < g_chinese_glyphs_count; i++) {
            if (g_chinese_glyphs[i].utf8[0] == code[0] &&
                g_chinese_glyphs[i].utf8[1] == code[1] &&
                g_chinese_glyphs[i].utf8[2] == code[2]) {
                return g_chinese_glyphs[i].matrix;
            }
        }
        return 0;
    }
    else if (code[0] >= 0x81U) {
        *bytes_consumed = 2U;
        for (i = 0; i < g_chinese_glyphs_count; i++) {
            if (g_chinese_glyphs[i].gbk[0] == code[0] &&
                g_chinese_glyphs[i].gbk[1] == code[1]) {
                return g_chinese_glyphs[i].matrix;
            }
        }
        return 0;
    }
    *bytes_consumed = 1U;
    return 0;
}

void lcd_draw_chinese_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg)
{
    const uint8_t *ptr = (const uint8_t *)text;
    while (*ptr) {
        if (*ptr < 128U) {
            lcd_draw_char(x, y + 1U, (char)*ptr, color, bg);
            x += 12U;
            ptr++;
        } else {
            uint8_t consumed = 0;
            const uint8_t *matrix = find_chinese_glyph(ptr, &consumed);
            if (matrix) {
                lcd_draw_chinese_char_16(x, y, matrix, color, bg);
            } else {
                lcd_fill_rect(x, y, 16U, 16U, bg);
                lcd_fill_rect(x + 1U, y + 1U, 14U, 14U, color);
                lcd_fill_rect(x + 3U, y + 3U, 10U, 10U, bg);
            }
            x += 16U;
            ptr += consumed;
        }
    }
}

void lcd_draw_chinese_text_32(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg)
{
    const uint8_t *ptr = (const uint8_t *)text;
    while (*ptr) {
        if (*ptr < 128U) {
            x += 16U;
            ptr++;
        } else {
            uint8_t consumed = 0;
            const uint8_t *matrix = find_chinese_glyph(ptr, &consumed);
            if (matrix) {
                lcd_draw_chinese_char_32(x, y, matrix, color, bg);
            }
            x += 32U;
            ptr += consumed;
        }
    }
}

static void lcd_draw_char_32x32(uint16_t x, uint16_t y, const uint8_t *matrix, uint16_t color_fill, uint16_t color_outline, uint16_t bg)
{
    uint16_t i, j;
    uint32_t rows[32];
    
    /* Load all rows into stack for fast neighbor lookups */
    for (i = 0; i < 32U; i++) {
        rows[i] = ((uint32_t)matrix[i << 2] << 24) |
                  ((uint32_t)matrix[(i << 2) + 1U] << 16) |
                  ((uint32_t)matrix[(i << 2) + 2U] << 8) |
                  (uint32_t)matrix[(i << 2) + 3U];
    }
    
    lcd_set_window(x, y, x + 31U, y + 31U);
    for (i = 0; i < 32U; i++) {
        for (j = 0; j < 32U; j++) {
            uint32_t mask = 0x80000000UL >> j;
            if (rows[i] & mask) {
                lcd_write_data(color_fill);
            } else {
                /* Check 8 neighbors in a 1-pixel radius */
                uint8_t has_neighbor = 0;
                int8_t di, dj;
                for (di = -1; di <= 1; di++) {
                    int16_t ni = (int16_t)i + di;
                    if (ni >= 0 && ni < 32) {
                        uint32_t r_val = rows[ni];
                        for (dj = -1; dj <= 1; dj++) {
                            int16_t nj = (int16_t)j + dj;
                            if (nj >= 0 && nj < 32) {
                                if (r_val & (0x80000000UL >> nj)) {
                                    has_neighbor = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (has_neighbor) {
                        break;
                    }
                }
                if (has_neighbor) {
                    lcd_write_data(color_outline);
                } else {
                    lcd_write_data(bg);
                }
            }
        }
    }
}

static void lcd_draw_char_16x32(uint16_t x, uint16_t y, const uint8_t *matrix, uint16_t color_fill, uint16_t color_outline, uint16_t bg)
{
    uint16_t i, j;
    uint16_t rows[32];
    
    for (i = 0; i < 32U; i++) {
        rows[i] = ((uint16_t)matrix[i << 1] << 8) | (uint16_t)matrix[(i << 1) + 1U];
    }
    
    lcd_set_window(x, y, x + 15U, y + 31U);
    for (i = 0; i < 32U; i++) {
        for (j = 0; j < 16U; j++) {
            uint16_t mask = 0x8000U >> j;
            if (rows[i] & mask) {
                lcd_write_data(color_fill);
            } else {
                uint8_t has_neighbor = 0;
                int8_t di, dj;
                for (di = -1; di <= 1; di++) {
                    int16_t ni = (int16_t)i + di;
                    if (ni >= 0 && ni < 32) {
                        uint16_t r_val = rows[ni];
                        for (dj = -1; dj <= 1; dj++) {
                            int16_t nj = (int16_t)j + dj;
                            if (nj >= 0 && nj < 16) {
                                if (r_val & (0x8000U >> nj)) {
                                    has_neighbor = 1;
                                    break;
                                }
                            }
                        }
                    }
                    if (has_neighbor) {
                        break;
                    }
                }
                if (has_neighbor) {
                    lcd_write_data(color_outline);
                } else {
                    lcd_write_data(bg);
                }
            }
        }
    }
}

void lcd_draw_snake_title_32(uint16_t x, uint16_t y, uint16_t color, uint16_t bg)
{
    uint16_t row;
    uint16_t col;
    const uint16_t layer_colors[TITLE_BITMAP_LAYERS] = {
        COLOR_WHITE,
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE,
    };

    (void)color;
    lcd_fill_rect(x, y, TITLE_BITMAP_WIDTH, TITLE_BITMAP_HEIGHT, bg);

    lcd_set_window(x, y,
                   (uint16_t)(x + TITLE_BITMAP_WIDTH - 1U),
                   (uint16_t)(y + TITLE_BITMAP_HEIGHT - 1U));
    for (row = 0U; row < TITLE_BITMAP_HEIGHT; row++) {
        for (col = 0U; col < TITLE_BITMAP_WIDTH; col++) {
            uint16_t pixel = bg;
            uint16_t byte_index = (uint16_t)(row * TITLE_BITMAP_BYTES_PER_ROW + (col >> 3));
            uint8_t bit_mask = (uint8_t)(0x80U >> (col & 7U));
            uint8_t layer;

            for (layer = 1U; layer < TITLE_BITMAP_LAYERS; layer++) {
                if ((g_title_bitmap[layer][byte_index] & bit_mask) != 0U) {
                    pixel = layer_colors[layer];
                    break;
                }
            }
            if ((g_title_bitmap[0][byte_index] & bit_mask) != 0U) {
                pixel = layer_colors[0];
            }
            lcd_write_data(pixel);
        }
    }
}

void lcd_draw_item(uint8_t x, uint8_t y, uint8_t type)
{
    uint16_t px = (uint16_t)(BOARD_X + x * CELL_SIZE + 1U);
    uint16_t py = (uint16_t)(BOARD_Y + y * CELL_SIZE + 1U);
    lcd_fill_rect(px, py, CELL_SIZE - 2U, CELL_SIZE - 2U, COLOR_BLACK);
    
    switch (type) {
        case 1U: /* ITEM_SPEED_UP - Yellow Lightning/Arrow */
            lcd_fill_rect(px + 4U, py + 1U, 2U, 8U, COLOR_YELLOW);
            lcd_fill_rect(px + 2U, py + 4U, 6U, 2U, COLOR_YELLOW);
            break;
        case 2U: /* ITEM_SLOW_DOWN - Blue snail shape */
            lcd_fill_rect(px + 2U, py + 4U, 6U, 4U, COLOR_BLUE);
            lcd_fill_rect(px + 6U, py + 2U, 2U, 2U, COLOR_CYAN);
            break;
        case 3U: /* ITEM_WALL_PASS - White shield / ring */
            lcd_fill_rect(px + 2U, py + 2U, 6U, 6U, COLOR_WHITE);
            lcd_fill_rect(px + 4U, py + 4U, 2U, 2U, COLOR_BLACK);
            break;
        case 4U: /* ITEM_DOUBLE_SCORE - Orange '2' */
            lcd_fill_rect(px + 2U, py + 2U, 6U, 2U, COLOR_ORANGE);
            lcd_fill_rect(px + 6U, py + 4U, 2U, 2U, COLOR_ORANGE);
            lcd_fill_rect(px + 2U, py + 6U, 6U, 2U, COLOR_ORANGE);
            break;
        case 5U: /* ITEM_SHORTEN - Lime scissor/minus */
            lcd_fill_rect(px + 2U, py + 4U, 6U, 2U, COLOR_LIME);
            break;
        default:
            break;
    }
}
