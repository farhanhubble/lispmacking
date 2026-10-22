#!/usr/bin/env python3
"""Export required symbols from the linked M6 or M7 user image."""
import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--services', action='store_true')
parser.add_argument('--nm', required=True)
parser.add_argument('--elf', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
symbols = {}
for line in subprocess.check_output([args.nm, '-n', str(args.elf)], text=True).splitlines():
    fields = line.split()
    if len(fields) == 3:
        symbols[fields[2]] = int(fields[0], 16)
exports = [('USER_ENTRY', 'user_main'), ('USER_RO_END', '__user_ro_end')]
if not args.services:
    exports += [('USER_SPIN_BEGIN', 'user_spin_begin'),
                ('USER_SPIN_END', 'user_spin_end'), ('USER_DATA_PROBE', 'data_instruction')]
args.output.write_text('/* Generated from user.elf; do not edit. */\n' + ''.join(
    f'#define {macro} 0x{symbols[symbol]:x}UL\n' for macro, symbol in exports))
