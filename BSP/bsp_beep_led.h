#ifndef BSP_BEEP_LED_H
#define BSP_BEEP_LED_H

#include <stdint.h>

void beep_led_init(void);
void led_on(void);
void led_off(void);
void raw_blink(uint8_t count);
void startup_after_systeminit_probe(void);
void boot_signal(uint8_t count);
void feedback_food(void);
void feedback_game_over(void);

#endif
