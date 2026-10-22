#!/usr/bin/env python3
"""Run M7 against a deterministic disk, plus a changed-disk negative control."""
import argparse
from pathlib import Path
import subprocess
import tempfile
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--qemu', default='qemu-system-riscv64')
p.add_argument('--kernel', type=Path, required=True)
p.add_argument('--disk', type=Path, required=True)
a = p.parse_args()
expected = ['disk Lisp through console service', 'client UART denied',
            'client VirtIO denied', 'client DMA denied', 'console fault wakes client',
            'block fault wakes client', 'client exit releases services',
            'client exit during block reply', 'console cannot access block device',
            'block cannot access UART', 'client kernel UART alias denied']
def run(disk, name):
    cmd = [a.qemu, '-machine', 'virt', '-m', '128M', '-smp', '1', '-bios', 'none',
           '-kernel', str(a.kernel), '-nographic', '-monitor', 'none',
           '-global', 'virtio-mmio.force-legacy=false']
    if disk is not None:
        cmd += ['-drive', f'file={disk},format=raw,if=none,id=disk',
                '-device', 'virtio-blk-device,drive=disk']
    try:
        r = subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=30)
    except subprocess.TimeoutExpired as e:
        a.kernel.with_name(name+'.log').write_bytes(e.stdout or b'')
        raise SystemExit('M7 timed out; see '+name+'.log')
    output = r.stdout.decode(errors='replace').replace('\r','')
    a.kernel.with_name(name+'.log').write_text(output)
    return r.returncode, output
code, output = run(a.disk, 'acceptance')
print(output, end='')
assert code == 0, 'QEMU failed'
assert [s for s in output.splitlines() if s.startswith('PASS:')] == ['PASS: '+s for s in expected]
assert output.splitlines()[-1:] == ['M7: PASS']
assert 'disk Lisp result: 42\n' in output
assert 'services: bounds, grants, device error and disk bytes verified\n' in output
assert 'client survived block fault\n' in output
assert 'AAAAAAAA' not in output, 'partial console payload leaked to device'
with tempfile.TemporaryDirectory() as folder:
    altered = Path(folder)/'changed.img'
    data = bytearray(a.disk.read_bytes())
    data[:512] = b'(+ 1 2)\n'.ljust(512, b'\0')
    altered.write_bytes(data)
    code, output = run(altered, 'changed-disk')
    assert code != 0 and 'world 0 exit=125' in output and 'M7: PASS' not in output, output
print('PASS: changed disk changes evaluation (negative control)')

code, output = run(None, 'missing-device')
assert code != 0 and 'world 0 exit=131' in output and 'block service reported I/O error' in output, output
assert 'M7: PASS' not in output
print('PASS: missing device returns service I/O error (negative control)')
