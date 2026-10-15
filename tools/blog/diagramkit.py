"""Small, dependency-free SVG vocabulary for publication diagrams.

Coordinates are logical pixels; export resolution is independent of layout.
Keep compositions in separate modules and reuse these semantic components.
"""
from dataclasses import dataclass
from html import escape
from math import atan2, cos, sin


@dataclass(frozen=True)
class Tone:
    ink: str
    wash: str

BLUE = Tone("#2459a6", "#e8f0ff")
AMBER = Tone("#995600", "#fff0ce")
VIOLET = Tone("#6d3bb0", "#f0e8ff")
TEAL = Tone("#146b60", "#ddf3eb")
CORAL = Tone("#a63d35", "#ffe8e1")
INK = "#202e3e"
MUTED = "#526174"
PAPER = "#fffdf7"
FONT = "Noto Sans"


class Diagram:
    def __init__(self, title, description, height, number, subtitle, article="BARE METAL ENLIGHTENMENT", compact=False):
        self.width, self.height = 800, height
        self.parts = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="800" height="{height}" viewBox="0 0 800 {height}" role="img" aria-labelledby="title desc">',
            f'<title id="title">{escape(title)}</title>',
            f'<desc id="desc">{escape(description)}</desc>',
        ]
        self.rect(0, 0, 800, height, PAPER, radius=0)
        if compact:
            self.text(36, 43, title, 25, bold=True)
            return
        self.text(36, 39, f"LISPMACKING  /  FIELD NOTES  /  {number}", 14, MUTED, bold=True)
        self.text(36, 82, title, 30, bold=True)
        self.text(36, 115, subtitle, 18, MUTED)
        self.line(36, height - 43, 764, height - 43, "#d9ddd9", 1)
        self.text(36, height - 18, article, 12, MUTED, bold=True)
        self.text(764, height - 18, "lispmacking", 12, MUTED, anchor="end")

    def rect(self, x, y, w, h, fill, stroke="none", radius=16, dashed=False):
        dash = ';stroke-dasharray:8,6' if dashed else ''
        self.parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" style="fill:{fill};stroke:{stroke};stroke-width:2{dash}"/>')

    def text(self, x, y, value, size=20, color=INK, bold=False, anchor="start"):
        self.parts.append(f'<text x="{x}" y="{y}" font-family="{FONT}, sans-serif" font-size="{size}" font-weight="{700 if bold else 400}" fill="{color}" text-anchor="{anchor}">{escape(value)}</text>')

    def lines(self, x, y, values, size=20, color=INK, leading=29):
        for i, value in enumerate(values):
            self.text(x, y + i * leading, value, size, color)

    def line(self, x1, y1, x2, y2, color=AMBER.ink, width=3):
        self.parts.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" style="stroke:{color};stroke-width:{width};stroke-linecap:round"/>')

    def arrow(self, x1, y1, x2, y2, color=AMBER.ink):
        angle = atan2(y2-y1, x2-x1)
        bx, by = x2-12*cos(angle), y2-12*sin(angle)
        self.line(x1, y1, bx, by, color)
        pts = [(x2, y2), (bx-5*sin(angle), by+5*cos(angle)), (bx+5*sin(angle), by-5*cos(angle))]
        self.parts.append('<polygon points="' + ' '.join(f'{x:.2f},{y:.2f}' for x,y in pts) + f'" fill="{color}"/>')

    def route(self, points, color=AMBER.ink):
        """An orthogonal connector whose final segment carries the arrowhead."""
        for start, end in zip(points[:-2], points[1:-1]):
            self.line(*start, *end, color)
        self.arrow(*points[-2], *points[-1], color)

    def layer(self, x, y, w, h, tone, label, detail):
        """A compact architectural layer, not a prose card."""
        self.rect(x, y, w, h, tone.wash, tone.ink, radius=6)
        self.rect(x, y, 6, h, tone.ink, radius=0)
        self.text(x+20, y+30, label, 22, tone.ink, bold=True)
        self.text(x+20, y+55, detail, 15, tone.ink)

    def chip(self, x, y, size, tone):
        """CPU symbol with pins; labels belong to the containing component."""
        self.rect(x, y, size, size, tone.wash, tone.ink, radius=4)
        self.rect(x+9, y+9, size-18, size-18, tone.ink, radius=2)
        for offset in range(9, size-3, 10):
            self.line(x-5, y+offset, x, y+offset, tone.ink, 2)
            self.line(x+size, y+offset, x+size+5, y+offset, tone.ink, 2)
            self.line(x+offset, y-5, x+offset, y, tone.ink, 2)
            self.line(x+offset, y+size, x+offset, y+size+5, tone.ink, 2)

    def card(self, x, y, w, h, tone, tag, title, lines, planned=False):
        self.rect(x, y, w, h, tone.wash, tone.ink, dashed=planned)
        self.text(x+22, y+29, tag, 13, tone.ink, bold=True)
        self.text(x+22, y+63, title, 24, tone.ink, bold=True)
        self.lines(x+22, y+96, lines, 19)

    def note(self, y, title, lines, tone=TEAL):
        self.rect(36, y, 728, 104, tone.wash)
        self.rect(36, y, 5, 104, tone.ink, radius=0)
        self.text(56, y+31, title, 20, tone.ink, bold=True)
        self.lines(56, y+60, lines, 18, leading=26)

    def svg(self):
        return "\n".join(self.parts + ["</svg>", ""])
