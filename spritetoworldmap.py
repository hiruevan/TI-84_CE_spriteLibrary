import sys
import os

def csv_sprite_to_map(input_file, output_file, map_name="world_map"):
    # Read the sprite
    with open(input_file, "r") as f:
        lines = [line.strip() for line in f if line.strip()]

    # Parse CSV into 2D list
    map_data = []
    for line in lines:
        row = [int(val.strip()) for val in line.split(",")]
        map_data.append(row)

    height = len(map_data)
    width = max(len(row) for row in map_data)

    # Pad rows to equal width
    for row in map_data:
        while len(row) < width:
            row.append(0)

    # Write the header
    with open(output_file, "w") as f:
        f.write(f"#ifndef {map_name.upper()}_HEAD\n")
        f.write(f"#define {map_name.upper()}_H {height}\n")
        f.write(f"#define {map_name.upper()}_W {width}\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"static const uint8_t {map_name}[{height}][{width}] = {{\n")
        for row in map_data:
            row_str = ", ".join(str(tile) for tile in row)
            f.write(f"    {{{row_str}}},\n")
        f.write("};\n\n")
        f.write(f"#endif // {map_name.upper()}_HEAD\n")

    print(f"Generated {output_file} ({width}x{height})")

# ------------------ CLI ------------------
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python csv_sprite_to_world_map.py input.sprite output.h [map_name]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    map_name = sys.argv[3] if len(sys.argv) >= 4 else "world_map"

    if not os.path.exists(input_file):
        print(f"Error: {input_file} does not exist.")
        sys.exit(1)

    csv_sprite_to_map(input_file, output_file, map_name)