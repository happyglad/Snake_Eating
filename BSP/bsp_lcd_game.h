#ifndef BSP_LCD_GAME_H
#define BSP_LCD_GAME_H

#include <stdint.h>

#define LCD_WIDTH          240U
#define LCD_HEIGHT         320U
#define GRID_COLS          20U
#define GRID_ROWS          15U
#define CELL_SIZE          12U
#define BOARD_X            0U
#define BOARD_Y            44U

#define COLOR_BLACK        0x0000U
#define COLOR_WHITE        0xFFFFU
#define COLOR_RED          0xF800U
#define COLOR_GREEN        0x07E0U
#define COLOR_BLUE         0x001FU
#define COLOR_YELLOW       0xFFE0U
#define COLOR_CYAN         0x07FFU
#define COLOR_GRAY         0x8410U
#define COLOR_DARK         0x18E3U
#define COLOR_ORANGE       0xFD20U
#define COLOR_PURPLE       0x780FU
#define COLOR_LIME         0xAFE5U

void lcd_game_init(void);
void lcd_game_init_trace(void (*trace_fn)(uint8_t stage));
void lcd_clear(uint16_t color);
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void lcd_backlight_set(uint8_t on);
void lcd_draw_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg);
void lcd_draw_number(uint16_t x, uint16_t y, uint32_t number, uint16_t color, uint16_t bg);
void lcd_draw_cell(uint8_t x, uint8_t y, uint16_t color);
void lcd_draw_snake_head(uint8_t x, uint8_t y, uint8_t dir);
void lcd_draw_snake_body(uint8_t x, uint8_t y);
void lcd_draw_snake_body_ex(uint8_t x, uint8_t y,
                            uint8_t prev_x, uint8_t prev_y,
                            uint8_t next_x, uint8_t next_y);
void lcd_draw_food(uint8_t x, uint8_t y);
void lcd_draw_obstacle(uint8_t x, uint8_t y);
void lcd_draw_touch_controls(void);
void lcd_draw_swipe_controls(void);
void lcd_draw_board(uint32_t score, uint32_t high_score);
void lcd_draw_board_ex(uint32_t score, uint32_t high_score, const char *mode);
void lcd_update_score(uint32_t score, uint32_t high_score);
void lcd_update_status(uint32_t score, uint32_t high_score, uint32_t duration);
void lcd_draw_chinese_char_16(uint16_t x, uint16_t y, const uint8_t *matrix, uint16_t color, uint16_t bg);
void lcd_draw_chinese_char_32(uint16_t x, uint16_t y, const uint8_t *matrix, uint16_t color, uint16_t bg);
void lcd_draw_chinese_text(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg);
void lcd_draw_chinese_text_32(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg);
void lcd_draw_snake_title_32(uint16_t x, uint16_t y, uint16_t color, uint16_t bg);
void lcd_draw_item(uint8_t x, uint8_t y, uint8_t type);
void lcd_update_status_ex(uint32_t score, uint32_t high_score, uint32_t duration, const char *item_text, uint32_t item_sec);

void lcd_show_start(uint32_t high_score);
void lcd_show_start_ex(uint32_t high_score, const char *mode);
void lcd_show_start_revamp(uint8_t focus, uint32_t high_score, const char *high_label);
void lcd_show_settings(uint8_t focus, const char *mode, uint8_t items_enabled);
void lcd_show_game_over(uint32_t score, uint32_t high_score);
void lcd_show_game_over_ex(uint32_t score, uint32_t high_score, uint32_t duration, const char *reason);

#endif
