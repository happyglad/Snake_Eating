#include "bsp_led.h"
#include "bsp_beep_led.h"
#include "bsp_key.h"
#include "bsp_lcd_game.h"
#include "bsp_touch.h"
#include "bsp_usart.h"
#include "snake_game.h"

/* Callback trace for LCD initialization steps (no operation to avoid LED flashing) */
static void trace_lcd_init_step(uint8_t stage)
{
    (void)stage;
}

/* Helper to print a number over USART1 for diagnostics */
static void usart1_send_number(uint32_t num)
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

int main(void)
{
    uint32_t t0, t1, t2;
    extern uint32_t SystemCoreClock;

    /* 1. Low-level core board initializations */
    LED_GPIO_Config();
    delay_init();
    beep_led_init();
    usart1_init();
    usart1_send_string("\r\n[System] USART Initialized successfully.");
    
    key_init();
    usart1_send_string("\r\n[System] GPIO Keys Initialized successfully.");
    
    /* 2. LCD and peripherals setup with profiling */
    t0 = millis();
    lcd_game_init_trace(trace_lcd_init_step);
    t1 = millis();
    usart1_send_string("\r\n[System] LCD Driver Initialized successfully.");

    touch_init();
    usart1_send_string("\r\n[System] Touch Driver Initialized successfully.");
    
    /* 3. Game initialization with profiling */
    snake_game_init();
    t2 = millis();
    usart1_send_string("\r\n[System] Snake Game Engine Ready.\r\n");
    
    /* Print system clock speed and boot timing diagnostics */
    usart1_send_string("\r\n[Clock] SystemCoreClock: ");
    usart1_send_number(SystemCoreClock);
    usart1_send_string(" Hz");
    
    usart1_send_string("\r\n[Profile] lcd_init: ");
    usart1_send_number(t1 - t0);
    usart1_send_string(" ms");
    
    usart1_send_string("\r\n[Profile] game_init: ");
    usart1_send_number(t2 - t1);
    usart1_send_string(" ms\r\n");
    
    /* Ensure all on-board RGB LEDs are kept OFF */
    LED_RGBOFF;

    /* 4. Infinite Game Loop scheduler */
    while (1) {
        KeyEvent key_evt;
        TouchEvent touch_evt;

        touch_set_playing(snake_game_is_playing());
        key_evt = key_scan_event();
        touch_evt = touch_scan_event();

        if (key_evt == KEY_EVENT_1) {
            snake_game_turn_left_or_start();
        } else if (key_evt == KEY_EVENT_2) {
            snake_game_turn_right_or_start();
        }

        switch (touch_evt) {
            case TOUCH_EVENT_UP:
                snake_game_on_command('U');
                break;
            case TOUCH_EVENT_DOWN:
                snake_game_on_command('D');
                break;
            case TOUCH_EVENT_LEFT:
                snake_game_on_command('L');
                break;
            case TOUCH_EVENT_RIGHT:
                snake_game_on_command('R');
                break;
            case TOUCH_EVENT_START:
                snake_game_on_command('S');
                touch_reset_state();
                break;
            case TOUCH_EVENT_MODE:
                snake_game_on_command('B');
                touch_reset_state();
                break;
            default:
                break;
        }

        /* Tick the game physics and state update */
        snake_game_tick();
    }
}
