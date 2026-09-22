import sys
import os

def parse_sprite(path):
    with open(path, "r") as f:
        lines = [line.strip() for line in f if line.strip()]

    sprite = []
    for line in lines:
        sprite.append([int(v.strip()) for v in line.split(",")])

    height = len(sprite)
    width = len(sprite[0]) if height > 0 else 0

    return sprite, width, height

def write_header(name, width, height, out_dir):
    h_path = os.path.join(out_dir, f"{name}.h")
    with open(h_path, "w") as f:
        f.write(f"""
#ifndef {name.upper()}_HEAD
#define {name.upper()}_H

#include <graphx.h>
#include <stdint.h>

extern gfx_sprite_t *{name};

#define {name.upper()}_WIDTH {width}
#define {name.upper()}_HEIGHT {height}

#endif // {name.upper()}_HEAD
        """)

def write_source(name, sprite, width, height, out_dir):
    c_path = os.path.join(out_dir, f"{name}.c")
    
    with open(c_path, "w") as f:
        f.write(f'#include "{name}.h"\n\n')
        
        # We define the sprite as a raw byte array to prevent C struct padding issues
        f.write(f"unsigned char {name}_data[] = {{\n")
        f.write(f"    {width}, {height}, // Width and Height headers\n")
        
        for y, row in enumerate(sprite):
            f.write("    ")
            bytes_in_row = [str(pixel & 0xFF) for pixel in row]
            f.write(", ".join(bytes_in_row))
            if y < len(sprite) - 1:
                f.write(",\n")
        
        f.write("\n};\n")
        # Map the extern sprite pointer to this raw data
        f.write(f"gfx_sprite_t *{name} = (gfx_sprite_t*){name}_data;\n")

def main():
    if len(sys.argv) < 4:
        print("Usage: python compilesprite.py input.sprite sprite_name out_dir")
        sys.exit(1)

    sprite_file = sys.argv[1]
    name = sys.argv[2]
    out_dir = sys.argv[3]

    os.makedirs(out_dir, exist_ok=True)

    sprite, w, h = parse_sprite(sprite_file)

    write_header(name, w, h, out_dir)
    write_source(name, sprite, w, h, out_dir)

    print(f"Generated {name}.c and {name}.h ({w}x{h})")

if __name__ == "__main__":
    main()