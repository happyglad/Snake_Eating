#ifndef FLASH_STORE_H
#define FLASH_STORE_H

#include <stdint.h>

uint32_t flash_load_high_score(void);
void flash_save_high_score(uint32_t score);
void flash_load_high_scores(uint32_t scores[4]);
void flash_save_high_scores(const uint32_t scores[4]);

#endif
