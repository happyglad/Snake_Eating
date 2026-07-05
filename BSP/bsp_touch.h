#ifndef BSP_TOUCH_H
#define BSP_TOUCH_H

#include <stdint.h>

typedef enum {
    TOUCH_EVENT_NONE = 0,
    TOUCH_EVENT_UP,
    TOUCH_EVENT_DOWN,
    TOUCH_EVENT_LEFT,
    TOUCH_EVENT_RIGHT,
    TOUCH_EVENT_START,
    TOUCH_EVENT_MODE
} TouchEvent;

void touch_init(void);
void touch_set_playing(uint8_t playing);
void touch_reset_state(void);
TouchEvent touch_scan_event(void);

#endif
