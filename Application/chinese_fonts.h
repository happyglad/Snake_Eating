#ifndef CHINESE_FONTS_H
#define CHINESE_FONTS_H

#include <stdint.h>

typedef struct {
    uint8_t gbk[2];       // GBK bytes
    uint8_t utf8[3];      // UTF-8 bytes
    uint8_t matrix[32];   // 16x16 font matrix
} ChineseGlyph16;

extern const ChineseGlyph16 g_chinese_glyphs[];
extern const uint16_t g_chinese_glyphs_count;

/* Custom 16x32 Snake Head and Tail Glyphs */
extern const uint8_t g_snake_head[64];
extern const uint8_t g_snake_tail[64];

/* 32x32 custom snake-themed glyphs for "贪吃蛇大冒险" */
extern const uint8_t g_snake_glyph_tan[128];
extern const uint8_t g_snake_glyph_chi[128];
extern const uint8_t g_snake_glyph_she[128];
extern const uint8_t g_snake_glyph_da[128];
extern const uint8_t g_snake_glyph_mao[128];
extern const uint8_t g_snake_glyph_xian[128];

#endif /* CHINESE_FONTS_H */
