from PIL import Image
import sys

def get_ti84_palette():
    """
    Generates the standard 256-color xLIBC palette.
    Format: 3 bits Red, 3 bits Green, 2 bits Blue (RRRGGGBB)
    """
    palette = []
    for i in range(256):
        # Extract RRRGGGBB bits
        r3 = (i >> 5) & 0x07
        g3 = (i >> 2) & 0x07
        b2 = i & 0x03

        # Scale to 0-255 using bit replication for accuracy
        r = (r3 << 5) | (r3 << 2) | (r3 >> 1)
        g = (g3 << 5) | (g3 << 2) | (g3 >> 1)
        b = (b2 << 6) | (b2 << 4) | (b2 << 2) | b2
        
        palette.extend((r, g, b))
    return palette

def sprite_to_png(sprite_path, out_path, scale=1):
    try:
        with open(sprite_path, "r") as f:
            lines = [line.strip() for line in f if line.strip()]

        # Convert text indices to flat list of integers
        data = []
        width = 0
        for line in lines:
            row = [int(v.strip()) for v in line.split(",") if v.strip()]
            width = len(row) # Assumption: rows are uniform width
            data.extend(row)

        height = len(data) // width

        # Create a palette-based image (Mode 'P')
        img = Image.new("P", (width, height))
        
        # Load the TI-84 palette into the image
        img.putpalette(get_ti84_palette())
        
        # Put the raw index data into the image
        img.putdata(data)

        # Convert to RGB for saving/scaling
        img = img.convert("RGB")

        if scale > 1:
            img = img.resize(
                (width * scale, height * scale),
                Image.NEAREST
            )

        img.save(out_path)
        print(f"Success! Debug PNG saved to {out_path} ({width}x{height})")

    except Exception as e:
        print(f"Error processing sprite: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python spritetopng.py input.sprite output.png [scale]")
        sys.exit(1)

    scale_val = int(sys.argv[3]) if len(sys.argv) >= 4 else 1
    sprite_to_png(sys.argv[1], sys.argv[2], scale_val)