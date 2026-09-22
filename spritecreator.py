from PIL import Image
import sys
import os

def get_graphx_palette():
    """
    Generate the graphx default palette (same format as TI-84 CE uses).
    This matches the palette in gfx_palette.
    """
    palette = []
    for i in range(256):
        # graphx uses RRRGGGBB format
        r3 = (i >> 5) & 0x07
        g3 = (i >> 2) & 0x07
        b2 = i & 0x03

        # Scale to 0-255 using proper bit replication
        r = (r3 * 255) // 7
        g = (g3 * 255) // 7
        b = (b2 * 255) // 3
        
        palette.append((r, g, b))
    return palette

def find_closest_palette_index(r, g, b, palette):
    """Find the closest color in the palette using Euclidean distance."""
    min_dist = float('inf')
    closest_idx = 0
    
    for idx, (pr, pg, pb) in enumerate(palette):
        dist = (r - pr)**2 + (g - pg)**2 + (b - pb)**2
        if dist < min_dist:
            min_dist = dist
            closest_idx = idx
    
    return closest_idx

def image_to_sprite(image_path, out_path, width=None, height=None):
    img = Image.open(image_path).convert("RGB")
    if width and height:
        img = img.resize((width, height), Image.NEAREST)

    w, h = img.size
    pixels = img.load()
    
    # Get the graphx palette
    palette = get_graphx_palette()

    with open(out_path, "w") as f:
        for y in range(h):
            row = []
            for x in range(w):
                r, g, b = pixels[x, y]
                # Find closest color in palette
                ti_col = find_closest_palette_index(r, g, b, palette)
                row.append(str(ti_col))
            f.write(", ".join(row) + "\n")

    print(f"Sprite written to {out_path} ({w}x{h})")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python spritecreator.py input.png output.sprite [width height]")
        sys.exit(1)

    input_img = sys.argv[1]
    output_sprite = sys.argv[2]

    if len(sys.argv) == 5:
        image_to_sprite(
            input_img,
            output_sprite,
            int(sys.argv[3]),
            int(sys.argv[4])
        )
    else:
        image_to_sprite(input_img, output_sprite)
