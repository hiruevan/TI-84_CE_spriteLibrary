#include "sprite_utils.h"

void setup_xlibc_palette(void) {
    for (int i = 0; i < 256; i++) {
        uint8_t r3 = (i >> 5) & 0x07;
        uint8_t g3 = (i >> 2) & 0x07;
        uint8_t b2 = i & 0x03;
        
        // Scale to 5-bit directly
        uint8_t r5 = (r3 << 2) | (r3 >> 1);
        uint8_t g5 = (g3 << 2) | (g3 >> 1);
        uint8_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);
        
        // Manually build 1555 format: 0RRRRRGGGGGBBBBB
        gfx_palette[i] = (r5 << 10) | (g5 << 5) | b5;
    }
}

void drawSprite(gfx_sprite_t *sprite, int x, int y) {
    gfx_Sprite(sprite, x, y);
}

void drawSpriteTransparent(gfx_sprite_t *sprite, int x, int y) {
    gfx_TransparentSprite(sprite, x, y);
}

void drawSpriteScaled(gfx_sprite_t *sprite, int x, int y, uint8_t scale) {
    uint8_t w = sprite->width;
    uint8_t h = sprite->height;
    
    // Data is 8-bit palette indices
    const uint8_t *pixels = sprite->data;

    for (uint8_t row = 0; row < h; row++) {
        for (uint8_t col = 0; col < w; col++) {
            // Get 8-bit palette index
            uint8_t paletteIndex = pixels[row * w + col];
            
            // Set color and draw scaled pixel
            gfx_SetColor(paletteIndex);
            gfx_FillRectangle(
                x + col * scale,
                y + row * scale,
                scale,
                scale
            );
        }
    }
}