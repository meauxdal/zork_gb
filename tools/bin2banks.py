#!/usr/bin/env python3
"""
bin2banks.py - Convert a binary blob into GBDK-2020 banked C source files.
Usage: python3 bin2banks.py <input.bin> <outdir/> <header.h> [--start-bank N]

Each output file gets a #pragma bank N directive so lcc places the data
in the correct MBC ROM bank automatically.
"""
import sys, os, math

def main():
    args = sys.argv[1:]
    start_bank = 1
    if '--start-bank' in args:
        i = args.index('--start-bank')
        start_bank = int(args[i+1])
        args = args[:i] + args[i+2:]

    if len(args) < 3:
        print(__doc__)
        sys.exit(1)

    in_file, out_dir, hdr_file = args[0], args[1], args[2]
    BANK_SIZE = 0x4000

    data = open(in_file, 'rb').read()
    # Pad to a multiple of BANK_SIZE
    pad = (BANK_SIZE - len(data) % BANK_SIZE) % BANK_SIZE
    data = data + b'\x00' * pad
    num_banks = len(data) // BANK_SIZE

    os.makedirs(out_dir, exist_ok=True)

    bank_names = []
    for b in range(num_banks):
        bank_num = start_bank + b
        name = f'zork_bank{bank_num}'
        bank_names.append((bank_num, name))
        chunk = data[b * BANK_SIZE : (b+1) * BANK_SIZE]
        out_path = os.path.join(out_dir, f'{name}.c')
        with open(out_path, 'w') as f:
            f.write(f'#pragma bank {bank_num}\n')
            f.write('#include <stdint.h>\n\n')
            f.write(f'/* Z-machine data bank {bank_num}: byte range '
                    f'0x{b*BANK_SIZE:05X}..0x{(b+1)*BANK_SIZE-1:05X} */\n')
            f.write(f'const unsigned char {name}_data[{BANK_SIZE}] = {{\n')
            for i in range(0, BANK_SIZE, 16):
                row = chunk[i:i+16]
                hex_vals = ','.join(f'0x{x:02X}' for x in row)
                comma = ',' if i + 16 < BANK_SIZE else ''
                f.write(f'    {hex_vals}{comma}\n')
            f.write('};\n')
        print(f'  Wrote {out_path}')

    with open(hdr_file, 'w') as f:
        f.write('#ifndef ZORK_DATA_H\n#define ZORK_DATA_H\n\n')
        f.write('#include <stdint.h>\n\n')
        f.write(f'#define ZORK_DATA_BANK_START  {start_bank}u\n')
        f.write(f'#define ZORK_DATA_NUM_BANKS   {num_banks}u\n')
        f.write(f'#define ZORK_BANK_SIZE        0x{BANK_SIZE:04X}u\n\n')
        for bank_num, name in bank_names:
            f.write(f'extern const unsigned char {name}_data[{BANK_SIZE}];\n')
        f.write('\n#endif /* ZORK_DATA_H */\n')
    print(f'  Wrote {hdr_file}')
    print(f'Done: {num_banks} banks, {len(data)} bytes total '
          f'(original {os.path.getsize(in_file)} bytes + {pad} padding)')

if __name__ == '__main__':
    main()
