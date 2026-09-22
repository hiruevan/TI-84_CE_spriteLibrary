#ifndef SPRITE_UTILS_H
#define SPRITE_UTILS_H

#include <graphx.h>
#include <stdint.h>

/* Draw a sprite normally */
void drawSprite(gfx_sprite_t *sprite, int x, int y);

/* Draw a sprite with transparency */
void drawSpriteTransparent(gfx_sprite_t *sprite, int x, int y);

/* Draw a scaled sprite (integer scale only) */
void drawSpriteScaled(gfx_sprite_t *sprite, int x, int y, uint8_t scale);

/* Setup the palette to match xlibc's default 256-color palette */
void setup_xlibc_palette(void);

#endif // SPRITE_UTILS_H