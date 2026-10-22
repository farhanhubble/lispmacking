#!/usr/bin/env python3
"""Build a deterministic four-sector fixture; source is loaded by VirtIO, not linked."""
import argparse
from pathlib import Path
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--source', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
source = a.source.read_bytes()
if not source or len(source) >= 512 or b'\0' in source:
    p.error('source must be 1..511 bytes without embedded NUL')
a.output.write_bytes(source.ljust(512, b'\0') + bytes((i*37+11) % 256 for i in range(512)) + bytes(1024))
