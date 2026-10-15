#!/usr/bin/env python3
"""Render publication SVG diagrams using only the Python standard library."""

from __future__ import annotations

from pathlib import Path
from xml.sax.saxutils import escape


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "blog" / "assets" / "diagrams"

BG = "#111719"
PANEL = "#1b2426"
CREAM = "#f3ead8"
MUTED = "#aeb9b6"
AMBER = "#e6a14a"
TEAL = "#54b9b1"
GRID = "#314044"


def begin(title: str, width: int = 1400, height: int = 800) -> list[str]:
    return [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img" aria-labelledby="title desc">',
        f'<title id="title">{escape(title)}</title>',
        '<desc id="desc">Technical diagram for the Lispmacking article series.</desc>',
        f'<rect width="100%" height="100%" fill="{BG}"/>',
        '<defs><filter id="shadow" x="-20%" y="-20%" width="140%" height="140%"><feDropShadow dx="0" dy="10" stdDeviation="12" flood-color="#000" flood-opacity=".28"/></filter></defs>',
    ]


def text(x: int, y: int, value: str, size: int = 28, color: str = CREAM,
         anchor: str = "middle", weight: int = 500) -> str:
    return (
        f'<text x="{x}" y="{y}" fill="{color}" font-family="Inter, system-ui, sans-serif" '
        f'font-size="{size}" font-weight="{weight}" text-anchor="{anchor}">{escape(value)}</text>'
    )


def mono(x: int, y: int, value: str, size: int = 25, color: str = CREAM,
         anchor: str = "middle") -> str:
    return (
        f'<text x="{x}" y="{y}" fill="{color}" font-family="ui-monospace, SFMono-Regular, Menlo, monospace" '
        f'font-size="{size}" text-anchor="{anchor}">{escape(value)}</text>'
    )


def box(x: int, y: int, w: int, h: int, stroke: str = GRID, fill: str = PANEL,
        radius: int = 18) -> str:
    return (
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" '
        f'fill="{fill}" stroke="{stroke}" stroke-width="3" filter="url(#shadow)"/>'
    )


def arrow(x1: int, y1: int, x2: int, y2: int, color: str = AMBER) -> list[str]:
    head = 13
    return [
        f'<line x1="{x1}" y1="{y1}" x2="{x2 - head}" y2="{y2}" stroke="{color}" stroke-width="5" stroke-linecap="round"/>',
        f'<path d="M {x2-head} {y2-head} L {x2} {y2} L {x2-head} {y2+head}" fill="none" stroke="{color}" stroke-width="5" stroke-linecap="round" stroke-linejoin="round"/>',
    ]


def save(name: str, body: list[str]) -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / name).write_text("\n".join(body + ["</svg>", ""]), encoding="utf-8")


def boot_path() -> None:
    svg = begin("Bare-metal RISC-V boot path")
    svg += [text(90, 85, "FROM RESET TO A SENTENCE", 22, AMBER, "start", 700)]
    labels = [
        ("QEMU reset", "CPU starts at _start"),
        ("Assembly", "stack + zero .bss"),
        ("kernel_main", "arena + VM setup"),
        ("UART", "one byte at a time"),
        ("Serial console", "Lisp machine booted."),
    ]
    xs = [90, 350, 610, 870, 1130]
    for idx, ((heading, detail), x) in enumerate(zip(labels, xs)):
        svg.append(box(x, 280, 190, 200, TEAL if idx in (0, 4) else GRID))
        svg.append(text(x + 95, 350, heading, 27, CREAM, weight=650))
        svg.append(text(x + 95, 403, detail, 20, MUTED))
        if idx < len(xs) - 1:
            svg += arrow(x + 200, 380, xs[idx + 1] - 10, 380)
    svg += [
        mono(700, 625, "0x80000000", 26, TEAL),
        text(700, 670, "QEMU virt RAM base — fixed by the linker map", 22, MUTED),
    ]
    save("boot-path.svg", svg)


def two_environments() -> None:
    svg = begin("One portable Lisp evaluator in two environments")
    svg += [text(700, 78, "ONE EVALUATOR, TWO EXECUTION ENVIRONMENTS", 22, AMBER, weight=700)]
    svg += [box(380, 135, 640, 150, TEAL), text(700, 195, "Portable Lisp VM", 34, CREAM, weight=700),
            mono(700, 242, "objects · reader · evaluator · printer", 22, MUTED)]
    svg += arrow(570, 305, 350, 405, TEAL)
    svg += arrow(830, 305, 1050, 405, TEAL)
    svg += [box(90, 405, 520, 250, AMBER), text(350, 470, "Hosted adapter", 31, CREAM, weight=700),
            mono(350, 520, "malloc + stdio", 25, AMBER),
            text(350, 570, "Normal OS process", 23, MUTED),
            text(350, 610, "Fast tests · sanitizers · debugger", 20, MUTED)]
    svg += [box(790, 405, 520, 250, AMBER), text(1050, 470, "RISC-V adapter", 31, CREAM, weight=700),
            mono(1050, 520, "arena + UART", 25, AMBER),
            text(1050, 570, "Bare-metal QEMU virt machine", 23, MUTED),
            text(1050, 610, "No OS · no C library", 20, MUTED)]
    svg += [text(700, 740, "Shared semantics; different binaries and machine obligations", 23, CREAM)]
    save("two-environments.svg", svg)


def compiler_pipeline() -> None:
    svg = begin("Lisp to native RV64 compiler pipeline")
    svg += [text(90, 82, "THE FIRST NATIVE BOOTSTRAP", 22, AMBER, "start", 700)]
    stages = [
        ("Function spec", "'((twice (x) (+ x x)))"),
        ("Lisp compiler", "walk + select + patch"),
        ("Lisp assembler", "R / I / S / B / J"),
        ("Code page", "checked 32-bit writes"),
        ("RV64 CPU", "fence.i + native call"),
    ]
    xs = [70, 335, 600, 865, 1130]
    for idx, ((heading, detail), x) in enumerate(zip(stages, xs)):
        svg.append(box(x, 230, 205, 210, TEAL if idx in (0, 4) else GRID))
        svg.append(text(x + 102, 300, heading, 27, CREAM, weight=650))
        svg.append(mono(x + 102, 356, detail, 14, MUTED))
        if idx < len(xs) - 1:
            svg += arrow(x + 215, 335, xs[idx + 1] - 10, 335)
    svg += [
        box(285, 555, 830, 120, GRID, "#151d1f", 14),
        text(700, 605, "C evaluator remains underneath", 27, CREAM, weight=650),
        text(700, 645, "It interprets the Lisp compiler; generated functions later run natively", 20, MUTED),
    ]
    save("compiler-pipeline.svg", svg)


def main() -> None:
    from prologue_diagrams import diagrams
    for name, diagram in diagrams().items():
        OUT.mkdir(parents=True, exist_ok=True)
        (OUT / f"{name}.svg").write_text(diagram.svg(), encoding="utf-8")
    boot_path()
    two_environments()
    compiler_pipeline()
    print(f"wrote diagrams to {OUT}")


if __name__ == "__main__":
    main()
