# Lispmacking field notes

This directory is the publication-neutral source for a narrative series about
building the experimental Lisp microkernel. Medium is the first intended
publisher, but every article is ordinary Markdown and every diagram is an SVG,
so the same material can be published on a personal site, Substack, DEV, or in
the repository itself.

The series follows one rule: each article tells one technical story. It begins
with a concrete problem, performs a small experiment, reaches one surprising
result, and stops before the next problem becomes a second article.

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

Diagrams and terminal images are reproducible and require only Python 3 plus
the normal project toolchain:

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
