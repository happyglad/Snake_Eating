#ifndef FLASH_STORE_H
#define FLASH_STORE_H

#include <stdint.h>

uint32_t flash_load_high_score(void);
void flash_save_high_score(uint32_t score);

#endif
