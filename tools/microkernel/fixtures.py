#!/usr/bin/env python3
"""Embed the actual Lisp integration programs, preserving their source files."""
import json
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[2]
Path(sys.argv[1]).write_text('/* Generated Lisp integration fixtures. */\n' + ''.join(
    f'static const char {name}_source[] = {json.dumps((root / "tests/microkernel" / (name + ".lisp")).read_text())};\n'
    for name in ('sender', 'receiver')))
