#!/usr/bin/env python3
"""Capture a real command or transcript and render it as publication SVG."""

from __future__ import annotations

import argparse
import html
import re
import shlex
import subprocess
import sys
import textwrap
from pathlib import Path


ANSI = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")


def clean(value: str) -> str:
    return ANSI.sub("", value).replace("\r", "")


def select_excerpt(value: str, start_pattern: str | None, max_lines: int) -> list[str]:
    lines = value.rstrip().splitlines()
    if start_pattern:
        pattern = re.compile(start_pattern)
        for index, line in enumerate(lines):
            if pattern.search(line):
                lines = lines[index:]
                break
    wrapped: list[str] = []
    for line in lines:
        pieces = textwrap.wrap(line, width=112, subsequent_indent="↳  ",
                               replace_whitespace=False, drop_whitespace=False)
        wrapped.extend(pieces or [""])
    lines = wrapped
    if len(lines) > max_lines:
        head = max_lines // 2
        tail = max_lines - head - 1
        lines = lines[:head] + ["⋯ output omitted; see the adjacent transcript ⋯"] + lines[-tail:]
    return lines or ["(command produced no output)"]


def render_svg(lines: list[str], title: str, command: str | None) -> str:
    char_width = 11
    line_height = 27
    width = max(1040, min(1800, max(len(line) for line in lines) * char_width + 100))
    height = 105 + (len(lines) + (2 if command else 0)) * line_height + 45
    body = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img" aria-labelledby="title desc">',
        f'<title id="title">{html.escape(title)}</title>',
        '<desc id="desc">Terminal output rendered from a real project command.</desc>',
        f'<rect width="100%" height="100%" rx="18" fill="#111719"/>',
        '<circle cx="34" cy="34" r="7" fill="#d76c62"/><circle cx="58" cy="34" r="7" fill="#e6a14a"/><circle cx="82" cy="34" r="7" fill="#54b9b1"/>',
        f'<text x="{width/2}" y="43" fill="#aeb9b6" font-family="Inter, system-ui, sans-serif" font-size="18" text-anchor="middle">{html.escape(title)}</text>',
        '<line x1="20" y1="64" x2="{}" y2="64" stroke="#314044" stroke-width="2"/>'.format(width - 20),
    ]
    y = 98
    if command:
        body.append(f'<text x="36" y="{y}" fill="#54b9b1" font-family="ui-monospace, SFMono-Regular, Menlo, monospace" font-size="19">$ {html.escape(command)}</text>')
        y += line_height * 2
    for line in lines:
        color = "#e6a14a" if "passed" in line.lower() else "#f3ead8"
        body.append(f'<text x="36" y="{y}" fill="{color}" font-family="ui-monospace, SFMono-Regular, Menlo, monospace" font-size="19" xml:space="preserve">{html.escape(line)}</text>')
        y += line_height
    body.append("</svg>")
    return "\n".join(body) + "\n"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--title", required=True)
    parser.add_argument("--output", required=True, help="Path without extension")
    parser.add_argument("--from-file", type=Path)
    parser.add_argument("--start-pattern")
    parser.add_argument("--max-lines", type=int, default=28)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    command_display: str | None = None
    if args.from_file:
        if args.command:
            raise SystemExit("choose --from-file or a command, not both")
        transcript = clean(args.from_file.read_text(encoding="utf-8"))
    else:
        command = args.command[1:] if args.command[:1] == ["--"] else args.command
        if not command:
            raise SystemExit("a command is required when --from-file is absent")
        command_display = shlex.join(command)
        completed = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, check=False)
        transcript = clean(completed.stdout)
        if completed.returncode:
            sys.stdout.write(transcript)
            return completed.returncode

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.with_suffix(".txt").write_text(transcript, encoding="utf-8")
    lines = select_excerpt(transcript, args.start_pattern, args.max_lines)
    output.with_suffix(".svg").write_text(
        render_svg(lines, args.title, command_display), encoding="utf-8"
    )
    print(f"wrote {output.with_suffix('.txt')} and {output.with_suffix('.svg')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
