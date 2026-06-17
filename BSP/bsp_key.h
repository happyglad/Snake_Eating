#ifndef BSP_KEY_H
#define BSP_KEY_H

#include <stdint.h>

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_1,
    KEY_EVENT_2
} KeyEvent;

void key_init(void);
KeyEvent key_scan_event(void);

#endif
