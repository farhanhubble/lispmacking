#!/usr/bin/env python3
"""Bounded QEMU acceptance test: exact per-scenario results, no grep false positives."""
import argparse
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--qemu', default='qemu-system-riscv64')
parser.add_argument('--kernel', type=Path, default=Path('build/microkernel/kernel.elf'))
parser.add_argument('--timeout', type=float, default=30)
args = parser.parse_args()
command = [args.qemu, '-machine', 'virt', '-m', '128M', '-smp', '1', '-bios', 'none',
           '-kernel', str(args.kernel), '-nographic', '-monitor', 'none']
try:
    result = subprocess.run(command, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=args.timeout)
except subprocess.TimeoutExpired as error:
    output = (error.stdout or b'').decode(errors='replace')
    args.kernel.with_name('acceptance.log').write_text(output)
    sys.exit('QEMU timed out (possible trap loop or IPC deadlock):\n' + output)
output = result.stdout.decode(errors='replace').replace('\r', '')
args.kernel.with_name('acceptance.log').write_text(output)
print(output, end='')
expected = [
    'PASS: isolated Lisp IPC', 'PASS: timer preemption',
    'PASS: kernel read denied cause=13', 'PASS: peer physical read denied cause=13',
    'PASS: UART access denied cause=13', 'PASS: text write denied cause=15',
    'PASS: data execute denied cause=12', 'PASS: privileged CSR denied cause=2',
    'PASS: stack guard fault cause=13', 'PASS: kernel execute denied cause=12',
    'PASS: peer exit wakes receiver',
    'PASS: peer fault wakes sender cause=13', 'PASS: no endpoint grant denies IPC',
]
actual = [line for line in output.splitlines() if line.startswith('PASS:')]
if result.returncode != 0 or actual != expected or output.splitlines()[-1:] != ['M6: PASS']:
    sys.exit('M6 acceptance failed: unexpected exit or scenario transcript')
for world in ('A', 'B'):
    if f'world {world}: Lisp IPC result t' not in output:
        sys.exit(f'Missing Lisp evaluation result from world {world}')
