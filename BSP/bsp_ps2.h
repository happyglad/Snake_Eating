#ifndef BSP_PS2_H
#define BSP_PS2_H

#include <stdint.h>

void ps2_init(void);
char ps2_scan_command(void);

#endif /* BSP_PS2_H */
