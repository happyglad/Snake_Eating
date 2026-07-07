#include "flash_store.h"
#include "stm32f103_min.h"

#define HIGH_SCORE_MAGIC       0x534E414BUL
#define HIGH_SCORE_PAGE_ADDR   0x0807F800UL
#define HIGH_SCORE_PAGE_SIZE   0x800UL
#define FLASH_KEY1             0x45670123UL
#define FLASH_KEY2             0xCDEF89ABUL

static void flash_wait(void)
{
    while (FLASH->SR & FLASH_SR_BSY) {
    }
}

static void flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
}

static void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

static void flash_erase_page(uint32_t addr)
{
    flash_wait();
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = addr;
    FLASH->CR |= FLASH_CR_STRT;
    flash_wait();
    FLASH->CR &= ~FLASH_CR_PER;
    FLASH->SR |= FLASH_SR_EOP;
}

static void flash_program_halfword(uint32_t addr, uint16_t data)
{
    flash_wait();
    FLASH->CR |= FLASH_CR_PG;
    *(__IO uint16_t *)addr = data;
    flash_wait();
    FLASH->CR &= ~FLASH_CR_PG;
    FLASH->SR |= FLASH_SR_EOP;
}

static void flash_program_word(uint32_t addr, uint32_t data)
{
    flash_program_halfword(addr, (uint16_t)(data & 0xFFFFU));
    flash_program_halfword(addr + 2U, (uint16_t)(data >> 16U));
}

uint32_t flash_load_high_score(void)
{
    uint32_t scores[4];
    flash_load_high_scores(scores);
    return scores[0];
}

void flash_save_high_score(uint32_t score)
{
    uint32_t scores[4] = {0U, 0U, 0U, 0U};

    flash_load_high_scores(scores);
    scores[0] = score;
    flash_save_high_scores(scores);
}

void flash_load_high_scores(uint32_t scores[4])
{
    uint8_t i;
    uint32_t magic = *(__IO uint32_t *)HIGH_SCORE_PAGE_ADDR;

    for (i = 0U; i < 4U; i++) {
        scores[i] = 0U;
    }

    if (magic != HIGH_SCORE_MAGIC) {
        return;
    }

    for (i = 0U; i < 4U; i++) {
        uint32_t score = *(__IO uint32_t *)(HIGH_SCORE_PAGE_ADDR + 4U + ((uint32_t)i * 4U));
        if (score <= 99990UL) {
            scores[i] = score;
        }
    }
}

void flash_save_high_scores(const uint32_t scores[4])
{
    uint8_t i;

    flash_unlock();
    flash_erase_page(HIGH_SCORE_PAGE_ADDR);
    flash_program_word(HIGH_SCORE_PAGE_ADDR, HIGH_SCORE_MAGIC);
    for (i = 0U; i < 4U; i++) {
        flash_program_word(HIGH_SCORE_PAGE_ADDR + 4U + ((uint32_t)i * 4U), scores[i]);
    }
    flash_lock();
}
