#ifndef SPRITE_UTILS_H
#define SPRITE_UTILS_H

#include <stdint.h>
#include <graphx.h>

/*
Draw sprites using gfx_Sprite and setup palette to match xlibc's default 256-color palette.

eg usage:
#include "sprite_utils.h"
setup_xlibc_palette();
gfx_Sprite(sprite, x, y);
*/

/* Setup the palette to match xlibc's default 256-color palette */
void setup_xlibc_palette(void);

#endif // SPRITE_UTILS_H