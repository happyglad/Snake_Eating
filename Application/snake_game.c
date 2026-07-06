#include "snake_game.h"
#include "bsp_beep_led.h"
#include "bsp_lcd_game.h"
#include "flash_store.h"
#include "stm32f103_min.h"

#define MAX_SNAKE_LEN     (GRID_COLS * GRID_ROWS)
#define INITIAL_SNAKE_LEN 4
#define BASE_INTERVAL_MS  220U
#define MIN_INTERVAL_MS   80U
#define SPEEDUP_FACTOR    3U   /* Decrease interval by 3ms per 10 points (1 food) */
#define OBSTACLE_COUNT    16U

typedef struct {
    uint8_t x;
    uint8_t y;
} Point;

typedef enum {
    STATE_START_SCREEN = 0,
    STATE_SETTINGS_SCREEN,
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

typedef enum {
    GAME_MODE_NORMAL = 0,
    GAME_MODE_BLOCKS,
    GAME_MODE_TIME_LIMIT
} GameMode;

typedef enum {
    END_REASON_NONE = 0,
    END_REASON_WALL,
    END_REASON_BODY,
    END_REASON_BLOCK,
    END_REASON_TIMEOUT
} EndReason;

typedef enum {
    ITEM_NONE = 0,
    ITEM_SPEED_UP,
    ITEM_SLOW_DOWN,
    ITEM_WALL_PASS,
    ITEM_DOUBLE_SCORE,
    ITEM_SHORTEN
} ItemType;

/* Item variables */
static Point g_item_pos;
static uint8_t g_item_type = ITEM_NONE;
static uint8_t g_item_active = 0;
static uint32_t g_item_spawn_time = 0;

static uint32_t g_speed_up_end_time = 0;
static uint32_t g_slow_down_end_time = 0;
static uint32_t g_wall_pass_end_time = 0;
static uint32_t g_double_score_end_time = 0;

/* Game variables */
static Point g_snake[MAX_SNAKE_LEN];
static uint16_t g_snake_len;
static Point g_food_pos;
static const Point g_obstacles[OBSTACLE_COUNT] = {
    {3, 3}, {4, 3}, {5, 3}, {14, 3}, {15, 3}, {16, 3},
    {3, 11}, {4, 11}, {5, 11}, {14, 11}, {15, 11}, {16, 11},
    {9, 5}, {10, 5}, {9, 9}, {10, 9}
};

static GameState g_game_state = STATE_START_SCREEN;
static GameMode g_game_mode = GAME_MODE_NORMAL;
static uint8_t g_main_start_focus = 0U; /* 0: 开始游戏, 1: 游戏设置 */
static uint8_t g_settings_focus = 0U;   /* 0: 模式, 1: 道具, 2: 返回 */
static uint8_t g_items_enabled = 1U;     /* 0: 关闭, 1: 开启 */

static SnakeDir g_current_dir = DIR_RIGHT;
static SnakeDir g_next_dir = DIR_RIGHT;
static uint8_t g_dir_changed_this_tick = 0;

static uint32_t g_score = 0;
static uint32_t g_high_score = 0;
static uint32_t g_rand_seed = 0x5A5A5A5AUL;
static uint32_t g_last_tick_time = 0;
static uint32_t g_game_start_time = 0;
static uint32_t g_last_status_second = 0;
static EndReason g_end_reason = END_REASON_NONE;
static volatile uint8_t g_start_requested = 0;

/* Pseudo-random generator (LCG) */
static uint16_t get_random(uint16_t max)
{
    g_rand_seed = g_rand_seed * 1103515245UL + 12345UL;
    return (uint16_t)((g_rand_seed / 65536UL) % max);
}

/* Helper to check if a point is on the snake's body */
static uint8_t is_point_on_snake(Point p)
{
    uint16_t i;
    for (i = 0; i < g_snake_len; i++) {
        if (g_snake[i].x == p.x && g_snake[i].y == p.y) {
            return 1U;
        }
    }
    return 0U;
}

static uint8_t is_point_on_snake_range(Point p, uint16_t count)
{
    uint16_t i;
    if (count > g_snake_len) {
        count = g_snake_len;
    }

    for (i = 0; i < count; i++) {
        if (g_snake[i].x == p.x && g_snake[i].y == p.y) {
            return 1U;
        }
    }
    return 0U;
}

static const char *game_mode_name(void)
{
    if (g_game_mode == GAME_MODE_BLOCKS) return "BLOCK";
    if (g_game_mode == GAME_MODE_TIME_LIMIT) return "TIME";
    return "NORMAL";
}

static const char *game_mode_event_name(void)
{
    if (g_game_mode == GAME_MODE_BLOCKS) return "blocks";
    if (g_game_mode == GAME_MODE_TIME_LIMIT) return "time_limit";
    return "normal";
}

static const char *end_reason_name(EndReason reason)
{
    switch (reason) {
        case END_REASON_WALL:    return "WALL";
        case END_REASON_BODY:    return "BODY";
        case END_REASON_BLOCK:   return "BLOCK";
        case END_REASON_TIMEOUT: return "TIMEOUT";
        default:                 return "END";
    }
}

static const char *end_reason_event_name(EndReason reason)
{
    switch (reason) {
        case END_REASON_WALL:    return "wall";
        case END_REASON_BODY:    return "body";
        case END_REASON_BLOCK:   return "block";
        case END_REASON_TIMEOUT: return "timeout";
        default:                 return "end";
    }
}

static uint32_t game_duration_seconds(void)
{
    if (g_game_state == STATE_START_SCREEN) {
        return 0U;
    }
    return (millis() - g_game_start_time) / 1000U;
}

static uint8_t is_point_on_obstacle(Point p)
{
    uint16_t i;
    if (g_game_mode != GAME_MODE_BLOCKS) {
        return 0U;
    }

    for (i = 0; i < OBSTACLE_COUNT; i++) {
        if (g_obstacles[i].x == p.x && g_obstacles[i].y == p.y) {
            return 1U;
        }
    }
    return 0U;
}

/* Spawns food at a random free cell */
static void spawn_food(void)
{
    Point p;
    uint32_t timeout = 0;
    
    do {
        p.x = (uint8_t)get_random(GRID_COLS);
        p.y = (uint8_t)get_random(GRID_ROWS);
        timeout++;
        /* Safeguard against infinite loop if board is completely full */
        if (timeout > 1000UL) {
            break;
        }
    } while (is_point_on_snake(p) || is_point_on_obstacle(p));

    g_food_pos = p;
    lcd_draw_food(g_food_pos.x, g_food_pos.y);
}

static void spawn_item(void)
{
    Point p;
    uint32_t timeout = 0;
    
    if (!g_items_enabled) {
        if (g_item_active) {
            lcd_draw_cell(g_item_pos.x, g_item_pos.y, COLOR_BLACK);
            g_item_active = 0;
        }
        return;
    }
    
    if (g_item_active) {
        lcd_draw_cell(g_item_pos.x, g_item_pos.y, COLOR_BLACK);
    }
    
    do {
        p.x = (uint8_t)get_random(GRID_COLS);
        p.y = (uint8_t)get_random(GRID_ROWS);
        timeout++;
        if (timeout > 1000UL) {
            return;
        }
    } while (is_point_on_snake(p) || is_point_on_obstacle(p) || (p.x == g_food_pos.x && p.y == g_food_pos.y));
    
    g_item_pos = p;
    g_item_active = 1U;
    g_item_type = (uint8_t)(get_random(5) + 1); /* 1 to 5 */
    g_item_spawn_time = millis();
    
    lcd_draw_item(g_item_pos.x, g_item_pos.y, g_item_type);
}

extern void usart1_send_string(const char *str);
extern void usart1_send_char(char ch);

static void game_usart_send_number(uint32_t num)
{
    char buf[11];
    uint8_t i = 0;
    if (num == 0) {
        usart1_send_string("0");
        return;
    }
    while (num && i < 10) {
        buf[i++] = (char)('0' + (num % 10));
        num /= 10;
    }
    while (i > 0) {
        usart1_send_char(buf[--i]);
    }
}

static const char *item_type_name(uint8_t type)
{
    switch (type) {
        case ITEM_SPEED_UP:     return "speed_up";
        case ITEM_SLOW_DOWN:    return "slow_down";
        case ITEM_WALL_PASS:    return "wall_pass";
        case ITEM_DOUBLE_SCORE: return "double";
        case ITEM_SHORTEN:      return "shorten";
        default:                return "none";
    }
}

static void send_item_event(uint8_t type)
{
    usart1_send_string("\r\nSNAKE,ITEM,type=");
    usart1_send_string(item_type_name(type));
    usart1_send_string(",score=");
    game_usart_send_number(g_score);
    usart1_send_string(",length=");
    game_usart_send_number(g_snake_len);
    usart1_send_string(",duration=");
    game_usart_send_number(game_duration_seconds());
    usart1_send_string("\r\n");
}

static void apply_item_effect(uint8_t type)
{
    uint32_t now = millis();
    
    send_item_event(type);
    
    switch (type) {
        case ITEM_SPEED_UP:
            g_speed_up_end_time = now + 10000UL;
            g_slow_down_end_time = 0;
            break;
        case ITEM_SLOW_DOWN:
            g_slow_down_end_time = now + 10000UL;
            g_speed_up_end_time = 0;
            break;
        case ITEM_WALL_PASS:
            g_wall_pass_end_time = now + 10000UL;
            break;
        case ITEM_DOUBLE_SCORE:
            g_double_score_end_time = now + 10000UL;
            break;
        case ITEM_SHORTEN:
            if (g_snake_len > INITIAL_SNAKE_LEN) {
                uint16_t old_len = g_snake_len;
                uint16_t reduce = 2U;
                uint16_t i;
                if (g_snake_len - reduce < INITIAL_SNAKE_LEN) {
                    g_snake_len = INITIAL_SNAKE_LEN;
                } else {
                    g_snake_len -= reduce;
                }
                /* Clear the removed tail segments from LCD */
                for (i = g_snake_len; i < old_len; i++) {
                    lcd_draw_cell(g_snake[i].x, g_snake[i].y, COLOR_BLACK);
                }
            }
            break;
        default:
            break;
    }
    
    feedback_food(); /* beep feedback */
}

static void send_start_event(void)
{
    usart1_send_string("\r\nSNAKE,START,mode=");
    usart1_send_string(game_mode_event_name());
    usart1_send_string("\r\n");
}

static void send_food_event(void)
{
    usart1_send_string("\r\nSNAKE,FOOD,score=");
    game_usart_send_number(g_score);
    usart1_send_string(",length=");
    game_usart_send_number(g_snake_len);
    usart1_send_string(",duration=");
    game_usart_send_number(game_duration_seconds());
    usart1_send_string("\r\n");
}

static void send_end_event(void)
{
    usart1_send_string("\r\nSNAKE,END,score=");
    game_usart_send_number(g_score);
    usart1_send_string(",high=");
    game_usart_send_number(g_high_score);
    usart1_send_string(",duration=");
    game_usart_send_number(game_duration_seconds());
    usart1_send_string(",reason=");
    usart1_send_string(end_reason_event_name(g_end_reason));
    usart1_send_string("\r\n");
}

static void draw_obstacles(void)
{
    uint16_t i;
    if (g_game_mode != GAME_MODE_BLOCKS) {
        return;
    }

    for (i = 0; i < OBSTACLE_COUNT; i++) {
        lcd_draw_obstacle(g_obstacles[i].x, g_obstacles[i].y);
    }
}

/* Start a new game round */
static void start_new_game(void)
{
    uint16_t i;
    uint32_t ta, tb, tc;
    
    g_score = 0;
    g_snake_len = INITIAL_SNAKE_LEN;
    g_end_reason = END_REASON_NONE;
    
    /* Reset item state variables */
    g_item_active = 0;
    g_item_type = ITEM_NONE;
    g_speed_up_end_time = 0;
    g_slow_down_end_time = 0;
    g_wall_pass_end_time = 0;
    g_double_score_end_time = 0;
    
    /* Start snake in the middle grid, horizontal */
    g_snake[0].x = 9; g_snake[0].y = 7;
    g_snake[1].x = 8; g_snake[1].y = 7;
    g_snake[2].x = 7; g_snake[2].y = 7;
    g_snake[3].x = 6; g_snake[3].y = 7;
    
    for (i = INITIAL_SNAKE_LEN; i < MAX_SNAKE_LEN; i++) {
        g_snake[i].x = 0;
        g_snake[i].y = 0;
    }
    
    g_current_dir = DIR_RIGHT;
    g_next_dir = DIR_RIGHT;
    g_dir_changed_this_tick = 0;
    
    /* Mix millis into seed to improve randomness */
    g_rand_seed ^= millis() + 0xFC3B9A17UL;
    
    /* Redraw board and first elements with profiling */
    ta = millis();
    lcd_clear(COLOR_BLACK);
    tb = millis();
    lcd_draw_board_ex(g_score, g_high_score, game_mode_name());
    tc = millis();
    
    usart1_send_string("\r\n[Profile] start_new_game clear: ");
    game_usart_send_number(tb - ta);
    usart1_send_string(" ms, draw_board: ");
    game_usart_send_number(tc - tb);
    usart1_send_string(" ms\r\n");
    
    draw_obstacles();
    lcd_draw_swipe_controls();

    /* Draw snake: head has direction marker, body uses a softer fill */
    lcd_draw_snake_head(g_snake[0].x, g_snake[0].y, (uint8_t)g_current_dir);
    for (i = 1; i < g_snake_len; i++) {
        lcd_draw_snake_body(g_snake[i].x, g_snake[i].y);
    }
    
    spawn_food();
    
    g_last_tick_time = millis();
    g_game_start_time = g_last_tick_time;
    g_last_status_second = 0U;
    g_game_state = STATE_PLAYING;
    send_start_event();
}

/* Handle game-over transition */
static void trigger_game_over(EndReason reason)
{
    g_game_state = STATE_GAME_OVER;
    g_end_reason = reason;
    
    /* Save high score to flash if updated */
    if (g_score > g_high_score) {
        g_high_score = g_score;
        flash_save_high_score(g_high_score);
    }
    
    send_end_event();
    lcd_show_game_over_ex(g_score, g_high_score, game_duration_seconds(), end_reason_name(g_end_reason));
    feedback_game_over();
}

/* Moves the snake by one cell, updates state & screen */
static void move_snake(void)
{
    Point head = g_snake[0];
    Point old_tail = g_snake[g_snake_len - 1];
    Point new_head;
    int16_t next_x = (int16_t)head.x;
    int16_t next_y = (int16_t)head.y;
    uint8_t ate_food = 0;
    uint16_t i;
    uint32_t now = millis();
    uint8_t wall_pass_active = (g_wall_pass_end_time > now) ? 1U : 0U;
    uint8_t double_score_active = (g_double_score_end_time > now) ? 1U : 0U;
    
    /* Commit next direction */
    g_current_dir = g_next_dir;
    g_dir_changed_this_tick = 0;
    
    /* Translate head position */
    switch (g_current_dir) {
        case DIR_UP:    next_y--; break;
        case DIR_DOWN:  next_y++; break;
        case DIR_LEFT:  next_x--; break;
        case DIR_RIGHT: next_x++; break;
    }
    
    /* Boundary check with Wall-Passing support */
    if (next_x < 0 || next_x >= (int16_t)GRID_COLS || next_y < 0 || next_y >= (int16_t)GRID_ROWS) {
        if (wall_pass_active) {
            if (next_x < 0) next_x = (int16_t)GRID_COLS - 1;
            else if (next_x >= (int16_t)GRID_COLS) next_x = 0;
            if (next_y < 0) next_y = (int16_t)GRID_ROWS - 1;
            else if (next_y >= (int16_t)GRID_ROWS) next_y = 0;
        } else {
            trigger_game_over(END_REASON_WALL);
            return;
        }
    }
    
    new_head.x = (uint8_t)next_x;
    new_head.y = (uint8_t)next_y;
    
    /* Normal movement may legally enter the current tail cell because it moves away. */
    if (is_point_on_snake_range(new_head, (uint16_t)(g_snake_len - 1U))) {
        trigger_game_over(END_REASON_BODY);
        return;
    }

    if (is_point_on_obstacle(new_head)) {
        trigger_game_over(END_REASON_BLOCK);
        return;
    }
    
    /* Food consumption check */
    if (new_head.x == g_food_pos.x && new_head.y == g_food_pos.y) {
        ate_food = 1U;
        if (g_snake_len < MAX_SNAKE_LEN) {
            g_snake_len++;
        }
        g_score += (double_score_active ? 20U : 10U);
        
        uint32_t elapsed = game_duration_seconds();
        uint32_t display_time = elapsed;
        if (g_game_mode == GAME_MODE_TIME_LIMIT) {
            display_time = (elapsed < 60U) ? (60U - elapsed) : 0U;
        }
        
        const char *item_abbr = "";
        uint32_t item_sec = 0;
        if (g_speed_up_end_time > now) {
            item_abbr = "加速";
            item_sec = (g_speed_up_end_time - now) / 1000U + 1U;
        } else if (g_slow_down_end_time > now) {
            item_abbr = "减速";
            item_sec = (g_slow_down_end_time - now) / 1000U + 1U;
        } else if (g_wall_pass_end_time > now) {
            item_abbr = "穿墙";
            item_sec = (g_wall_pass_end_time - now) / 1000U + 1U;
        } else if (g_double_score_end_time > now) {
            item_abbr = "双倍";
            item_sec = (g_double_score_end_time - now) / 1000U + 1U;
        }
        
        lcd_update_status_ex(g_score, g_high_score, display_time, item_abbr, item_sec);
        send_food_event();
        feedback_food();
        
        /* 40% chance to spawn special item */
        if (get_random(10) < 4) {
            spawn_item();
        }
    }
    
    /* Item consumption check */
    if (g_item_active && new_head.x == g_item_pos.x && new_head.y == g_item_pos.y) {
        apply_item_effect(g_item_type);
        g_item_active = 0U;
        old_tail = g_snake[g_snake_len - 1U]; /* Update tail in case shortened */
    }
    
    /* Shift body coordinates */
    for (i = (uint16_t)(g_snake_len - 1U); i > 0U; i--) {
        g_snake[i] = g_snake[i - 1U];
    }
    g_snake[0] = new_head;
    
    if (ate_food) {
        /* Redraw: color new head green and turn old head into cyan body */
        lcd_draw_snake_head(g_snake[0].x, g_snake[0].y, (uint8_t)g_current_dir);
        lcd_draw_snake_body(g_snake[1].x, g_snake[1].y);
        spawn_food();
    } else {
        /* Normal step: clear old tail cell, draw new head, and turn old head into body */
        lcd_draw_cell(old_tail.x, old_tail.y, COLOR_BLACK);
        lcd_draw_snake_body(g_snake[1].x, g_snake[1].y);
        lcd_draw_snake_head(g_snake[0].x, g_snake[0].y, (uint8_t)g_current_dir);
    }
}

/* Public functions */

void snake_game_init(void)
{
    g_rand_seed = 0x19B3E2D5UL ^ millis();
    g_high_score = flash_load_high_score();
    g_game_state = STATE_START_SCREEN;
    g_start_requested = 0;
    g_main_start_focus = 0U;
    g_settings_focus = 0U;
    lcd_show_start_revamp(g_main_start_focus, g_high_score);
}

void snake_game_on_command(char cmd)
{
    SnakeDir target = g_current_dir;
    uint8_t is_valid = 0;

    /* Handle UI navigation commands in menus */
    if (g_game_state == STATE_START_SCREEN) {
        if (cmd == 'S' || cmd == 's') {
            if (g_main_start_focus == 0U) {
                g_start_requested = 1U;
            } else {
                g_game_state = STATE_SETTINGS_SCREEN;
                g_settings_focus = 0U;
                lcd_show_settings(g_settings_focus, game_mode_name(), g_items_enabled);
            }
            return;
        }
        if (cmd == 'B' || cmd == 'b' || cmd == 'M' || cmd == 'm') {
            g_main_start_focus = (g_main_start_focus == 0U) ? 1U : 0U;
            lcd_show_start_revamp(g_main_start_focus, g_high_score);
            return;
        }
    }
    else if (g_game_state == STATE_SETTINGS_SCREEN) {
        if (cmd == 'S' || cmd == 's') {
            if (g_settings_focus == 0U) {
                /* Cycle mode */
                if (g_game_mode == GAME_MODE_NORMAL) {
                    g_game_mode = GAME_MODE_BLOCKS;
                } else if (g_game_mode == GAME_MODE_BLOCKS) {
                    g_game_mode = GAME_MODE_TIME_LIMIT;
                } else {
                    g_game_mode = GAME_MODE_NORMAL;
                }
                lcd_show_settings(g_settings_focus, game_mode_name(), g_items_enabled);
            } else if (g_settings_focus == 1U) {
                /* Toggle items */
                g_items_enabled = (g_items_enabled == 0U) ? 1U : 0U;
                lcd_show_settings(g_settings_focus, game_mode_name(), g_items_enabled);
            } else {
                /* Back to main start screen */
                g_game_state = STATE_START_SCREEN;
                lcd_show_start_revamp(g_main_start_focus, g_high_score);
            }
            return;
        }
        if (cmd == 'B' || cmd == 'b' || cmd == 'M' || cmd == 'm') {
            g_settings_focus = (g_settings_focus + 1U) % 3U;
            lcd_show_settings(g_settings_focus, game_mode_name(), g_items_enabled);
            return;
        }
    }
    else if (g_game_state == STATE_GAME_OVER) {
        if (cmd == 'S' || cmd == 's') {
            g_start_requested = 1U;
            return;
        }
        if (cmd == 'B' || cmd == 'b' || cmd == 'M' || cmd == 'm') {
            g_game_state = STATE_START_SCREEN;
            g_main_start_focus = 0U;
            lcd_show_start_revamp(g_main_start_focus, g_high_score);
            return;
        }
    }

    if (g_game_state != STATE_PLAYING || g_dir_changed_this_tick) {
        return;
    }

    switch (cmd) {
        case 'U':
        case 'u':
            if (g_current_dir != DIR_DOWN) {
                target = DIR_UP;
                is_valid = 1U;
            }
            break;
        case 'D':
        case 'd':
            if (g_current_dir != DIR_UP) {
                target = DIR_DOWN;
                is_valid = 1U;
            }
            break;
        case 'L':
        case 'l':
            if (g_current_dir != DIR_RIGHT) {
                target = DIR_LEFT;
                is_valid = 1U;
            }
            break;
        case 'R':
        case 'r':
            if (g_current_dir != DIR_LEFT) {
                target = DIR_RIGHT;
                is_valid = 1U;
            }
            break;
        default:
            break;
    }
    
    if (is_valid) {
        g_next_dir = target;
        g_dir_changed_this_tick = 1U;
    }
}

uint8_t snake_game_is_playing(void)
{
    return (g_game_state == STATE_PLAYING) ? 1U : 0U;
}

void snake_game_turn_left_or_start(void)
{
    SnakeDir target = g_next_dir;

    if (g_game_state == STATE_START_SCREEN || g_game_state == STATE_SETTINGS_SCREEN || g_game_state == STATE_GAME_OVER) {
        snake_game_on_command('B');
        return;
    }
    
    if (g_game_state == STATE_PLAYING && !g_dir_changed_this_tick) {
        switch (g_next_dir) {
            case DIR_UP:    target = DIR_LEFT;  break;
            case DIR_LEFT:  target = DIR_DOWN;  break;
            case DIR_DOWN:  target = DIR_RIGHT; break;
            case DIR_RIGHT: target = DIR_UP;    break;
        }
        g_next_dir = target;
        g_dir_changed_this_tick = 1U;
    }
}

void snake_game_turn_right_or_start(void)
{
    SnakeDir target = g_next_dir;

    if (g_game_state == STATE_START_SCREEN || g_game_state == STATE_SETTINGS_SCREEN || g_game_state == STATE_GAME_OVER) {
        snake_game_on_command('S');
        return;
    }
    
    if (g_game_state == STATE_PLAYING && !g_dir_changed_this_tick) {
        switch (g_next_dir) {
            case DIR_UP:    target = DIR_RIGHT; break;
            case DIR_RIGHT: target = DIR_DOWN;  break;
            case DIR_DOWN:  target = DIR_LEFT;  break;
            case DIR_LEFT:  target = DIR_UP;    break;
        }
        g_next_dir = target;
        g_dir_changed_this_tick = 1U;
    }
}

void snake_game_tick(void)
{
    uint32_t elapsed;
    uint32_t now = millis();

    /* Handle start/restart requests */
    if (g_start_requested) {
        g_start_requested = 0U;
        if (g_game_state == STATE_START_SCREEN || g_game_state == STATE_GAME_OVER) {
            start_new_game();
            return;
        }
    }
    
    if (g_game_state != STATE_PLAYING) {
        return;
    }
    
    /* Handle Time-Limited Challenge 60s timeout */
    elapsed = game_duration_seconds();
    if (g_game_mode == GAME_MODE_TIME_LIMIT && elapsed >= 60U) {
        trigger_game_over(END_REASON_TIMEOUT);
        return;
    }
    
    /* Handle item spawn timeout on board (expires after 10s) */
    if (g_item_active && (now - g_item_spawn_time >= 10000UL)) {
        g_item_active = 0U;
        lcd_draw_cell(g_item_pos.x, g_item_pos.y, COLOR_BLACK);
    }
    
    /* Calculate interval with speedup logic and active items */
    uint8_t speed_up_active = (g_speed_up_end_time > now) ? 1U : 0U;
    uint8_t slow_down_active = (g_slow_down_end_time > now) ? 1U : 0U;
    
    uint32_t speed_deduction = (g_score / 10U) * SPEEDUP_FACTOR;
    int32_t interval = (int32_t)BASE_INTERVAL_MS - speed_deduction;
    if (interval < (int32_t)MIN_INTERVAL_MS) {
        interval = (int32_t)MIN_INTERVAL_MS;
    }
    
    if (speed_up_active) {
        interval -= 60;
    }
    if (slow_down_active) {
        interval += 80;
    }
    
    if (interval < (int32_t)MIN_INTERVAL_MS - 30) {
        interval = (int32_t)MIN_INTERVAL_MS - 30;
    }
    if (interval > 450) {
        interval = 450;
    }
    
    if (millis() - g_last_tick_time >= (uint32_t)interval) {
        g_last_tick_time = millis();
        move_snake();
    }

    elapsed = game_duration_seconds();
    if (elapsed != g_last_status_second) {
        g_last_status_second = elapsed;
        
        uint32_t display_time = elapsed;
        if (g_game_mode == GAME_MODE_TIME_LIMIT) {
            display_time = (elapsed < 60U) ? (60U - elapsed) : 0U;
        }
        
        const char *item_abbr = "";
        uint32_t item_sec = 0;
        if (g_speed_up_end_time > now) {
            item_abbr = "加速";
            item_sec = (g_speed_up_end_time - now) / 1000U + 1U;
        } else if (g_slow_down_end_time > now) {
            item_abbr = "减速";
            item_sec = (g_slow_down_end_time - now) / 1000U + 1U;
        } else if (g_wall_pass_end_time > now) {
            item_abbr = "穿墙";
            item_sec = (g_wall_pass_end_time - now) / 1000U + 1U;
        } else if (g_double_score_end_time > now) {
            item_abbr = "双倍";
            item_sec = (g_double_score_end_time - now) / 1000U + 1U;
        }
        
        lcd_update_status_ex(g_score, g_high_score, display_time, item_abbr, item_sec);
    }
}

uint8_t snake_game_items_enabled(void)
{
    return g_items_enabled;
}
