#include "sprite_utils.h"

void setup_xlibc_palette(void) {
    for (int i = 0; i < 256; i++) {
        uint8_t r = (i >> 5) & 7;
        uint8_t g = (i >> 2) & 7;
        uint8_t b = i & 3;
        
        gfx_palette[i] = ((r << 2) << 10) | ((g << 2) << 5) | (b << 3);
    }
}