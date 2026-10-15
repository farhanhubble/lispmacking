# Lispmacking field notes

This directory is the publication-neutral source for a narrative series about
building the experimental Lisp microkernel. Medium is the first intended
publisher, but every article is ordinary Markdown and every diagram is an SVG,
so the same material can be published on a personal site, Substack, DEV, or in
the repository itself.

The series begins with [Bare Metal Enlightenment](articles/00-bare-metal-enlightenment.md),
the question about SectorLISP that connected bare-metal programming and Lisp
and started the project. Every article after it tells one technical story: a
concrete problem, a small experiment, one surprising result, and a stopping
point before the next problem becomes a second article.

## Contents

- [Series plan](series-plan.md) — the narrative arc from first boot through
  user-space services.
- [Voice and editorial guide](style-guide.md) — observations from Farhan's
  earlier writing and constraints for this series.
- [Publishing checklist](publishing-checklist.md) — technical, visual, and
  distribution checks used before an issue is closed.
- [Publication log](publication-log.md) — canonical URLs, cross-posts, dates,
  and source commits.
- [Article drafts](articles/) — publication drafts with asset callouts.
- `assets/artwork/` — editorial hero artwork.
- `assets/diagrams/` — deterministic, editable SVG diagrams.
- `assets/terminal/` — terminal images rendered from real command output.

## Rebuilding visual evidence

SVG diagrams require only Python 3. Terminal captures also require the normal
project toolchain. The prologue PNG exports use uv, CairoSVG 2.8.2, the system
Cairo library, and Noto Sans:

```sh
make blog-diagrams
make blog-terminal-assets
```

`make blog-terminal-assets` runs the real hosted and QEMU test commands. It
saves both the plain transcript and an SVG rendering, so published images can
always be checked against their source.

## Publication workflow

1. Choose the next story from `series-plan.md`.
2. Verify every technical statement against the current repository.
3. Run the demonstration and rebuild its terminal asset.
4. Review the draft as a story: tension, experiment, reveal, consequence.
5. Export images as PNG only if a publisher cannot ingest SVG; retain SVG as
   the source.
6. Add platform-specific title, subtitle, canonical URL, and tags only at
   publication time.
7. Record the live URL and source commit in `publication-log.md`.

## Reusable publication diagrams

The opening article uses one architectural diagram: QEMU encloses the emulated
hardware and our implementation layers, with a separate generated-code path.
It has an SVG for Markdown hosts and a 1600-pixel-wide PNG for Medium beside it
in `assets/diagrams/`.

```sh
make blog-prologue-assets   # SVG + PNG, no QEMU required
make blog-check-diagrams   # read-only SVG freshness check
python3 tools/blog/export_diagrams.py --format svg
```

`make blog-assets` includes these exports alongside the existing figures and
terminal evidence. `uv` fetches the pinned CairoSVG release on first use. Cairo
and Noto Sans must be installed separately; the SVG generator itself has no
third-party Python dependencies. SVG fonts are not embedded. Use the PNGs for
identical typography on publishing platforms; font and Cairo versions can
change raster output across machines.

The reusable layers are:

- `tools/blog/diagramkit.py`: palette, typography, cards, planned boundaries,
  direction-aware arrows, accessible titles/descriptions, and figure framing.
- `tools/blog/prologue_diagrams.py`: article-specific compositions returning
  `Diagram` objects, registered by stable output name in `diagrams()`.
- `tools/blog/export_diagrams.py`: shared SVG/PNG export and freshness checking.
  `--output-dir` supports previews; `--width` controls PNG resolution.

For another article, reuse `Diagram`, pass its title through the `article`
argument, and add its compositions to the mapping consumed by the exporter.
Keep reusable primitives in the toolkit and article prose in compositions.
The other articles still use the legacy figures in `render_diagrams.py`.

Use diagrams for structure, containment, relationships, and execution paths.
Keep explanations in normal article prose; do not turn paragraphs into cards.
A useful diagram should convey something through spatial arrangement that a
list of sentences would lose.

Keep images close to the paragraph that introduces their question, with a
caption explaining the implication. In Medium, upload the adjacent PNG and
copy the Markdown caption and alt text into the corresponding image fields.
Check figures at article width and phone width; split a dense figure rather
than reducing its text size. A resolution increase does not enlarge labels.

Before publishing, run the freshness check and inspect the exported PNGs for
clipping, font substitutions, arrow direction, and visible planned boundaries.
The check compares SVG source output; it does not validate PNG freshness or
replace visual review.
