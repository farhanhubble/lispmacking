# Publishing checklist

Use this checklist after an article issue reaches technical review. The source
Markdown remains canonical; publication platforms are distribution targets.

## Story

- The opening contains a concrete tension, failure, or result.
- The article has one principal technical reveal.
- New terms arrive after the reader has a reason to need them.
- The experiment is small enough to follow without reconstructing the project.
- The honest-boundary section says what the result does *not* prove.
- The final paragraph opens one door rather than summarizing every section.

## Evidence

- Commands were run against the commit the article links to.
- Terminal images were rebuilt with `make blog-terminal-assets`.
- The adjacent `.txt` transcripts match the terminal images.
- Diagrams were rebuilt with `make blog-diagrams`.
- Implemented and planned components are clearly distinguished.
- Addresses, instruction words, register names and ABI details were checked.

## Visuals

- The hero works at a wide crop and as a small thumbnail.
- No generated image contains fake code, labels or logos.
- Every diagram has readable typography at article width.
- Every image has useful alt text and a caption that directs attention.
- PNG derivatives, if required, are exported from the committed source asset.

## Distribution

- Title and subtitle fit the destination without changing the article's claim.
- The canonical URL has been chosen before cross-posting.
- Code formatting, links and image captions have been checked in preview.
- The published URL and date are recorded in `publication-log.md`.
- The article issue is closed only after its live page has been checked.

