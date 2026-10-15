#!/usr/bin/env python3
"""Render/check the prologue's SVGs and export PNGs using CairoSVG.

No shell interpolation; output paths and executable arguments stay separate.
"""
import argparse
from pathlib import Path
from prologue_diagrams import diagrams

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--format", choices=("svg", "png", "both"), default="both")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "blog/assets/diagrams")
    parser.add_argument("--width", type=int, default=1600, help="PNG width in pixels (default: 2x)")
    parser.add_argument("--check", action="store_true", help="Check committed SVGs without writing files")
    args = parser.parse_args()
    if args.width < 1:
        parser.error("--width must be positive")
    figures = diagrams()
    if args.check:
        stale = [name for name, figure in figures.items()
                 if not (args.output_dir / f"{name}.svg").exists()
                 or (args.output_dir / f"{name}.svg").read_text(encoding="utf-8") != figure.svg()]
        if stale:
            parser.exit(1, "Stale or missing SVGs: " + ", ".join(stale) + "\n")
        print(f"{len(figures)} SVGs match their sources")
        return
    if args.format != "svg":
        try:
            import cairosvg
        except ImportError:
            parser.error("PNG export needs CairoSVG; run make blog-prologue-assets, or use --format svg")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, figure in figures.items():
        svg = figure.svg()
        if args.format in ("svg", "both"):
            (args.output_dir / f"{name}.svg").write_text(svg, encoding="utf-8")
        if args.format in ("png", "both"):
            cairosvg.svg2png(bytestring=svg.encode(), output_width=args.width,
                            write_to=str(args.output_dir / f"{name}.png"))
        print(f"Rendered {name} ({args.format})")


if __name__ == "__main__":
    main()
