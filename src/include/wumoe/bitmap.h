#ifndef WUMOE_BITMAP_H
#define WUMOE_BITMAP_H

#include <wumoe/types.h>

typedef struct bitmap_t {
    u8 *bits;
    u32 length;
    u32 offset;
} bitmap_t;

void bitmap_init(bitmap_t *map, char *bits, u32 length, u32 offset);

void bitmap_make(bitmap_t *map, char *bits, u32 length, u32 offset);

bool bitmap_test(bitmap_t *map, u32 index);

void bitmap_set(bitmap_t *map, u32 index, bool value);

int bitmap_scan(bitmap_t *map, u32 count);

#endif
