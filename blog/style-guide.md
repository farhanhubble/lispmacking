# Voice and editorial guide

This guide is based on three published pieces: *Rediscovering Semantic
Segmentation*, *Graceful API Failure 101 for Data Scientists*, and
*Reinventing P-values (Part 1)*.

## What already sounds like Farhan

- Begin with a misconception or an inconvenient question. The opening should
  create a reason to keep reading before introducing architecture.
- Reconstruct ideas from small, familiar pieces. A bedroom histogram can lead
  to a p-value; a single UART byte can lead to a Lisp machine.
- Prefer a running example to a taxonomy. Add vocabulary when the reader has
  already encountered the thing being named.
- Use short summary statements as hinges. They should compress the previous
  section and make the next step feel inevitable.
- Be candid about failed or incomplete designs. A bug, restriction, or ugly
  intermediate result is part of the plot rather than an embarrassment.
- Use code and diagrams as evidence. Explain what the reader should notice;
  never leave a large block to speak for itself.
- End by saying what the experiment made possible, then point at the next
  unresolved problem.

## Anchor for the opening article

Farhan's revised prologue introduction is the voice reference: concrete
experience with Scheme, Common Lisp, C and assembly, a modest LED-blinking
comparison, and a direct question. Carry that voice through the explanation.

- Use first person for the author's experience and “we” for project work.
- Let humour come from a concrete detail already in the story; do not invent
  personal anecdotes or reactions to make the narrative livelier.
- Explain why each implementation step follows from the previous one.
- Prefer connected paragraphs over repeated dramatic one-line reveals.
- Keep limitations specific and conversational, without turning the roadmap
  into a manifesto. Leave technical structure to the architecture diagram.

## Adjustments for this series

The older long-form tutorial style is valuable, but this series should read
more like linked chapters:

- target 900–1,400 words and a 5–8 minute reading time;
- keep one principal technical reveal per article;
- use no more than two substantial code excerpts;
- define precise terms such as *machine mode*, *ABI*, or *capability* without
  translating them into vague metaphors;
- use the metaphor to establish intuition, then state where it stops working;
- avoid claims of self-hosting until the exact self-hosted layer is named;
- avoid “revolutionary”, “magic”, “AI-powered”, and strings of fashionable
  adjectives;
- prefer “we made the code page writable and executable” to “we unlocked a
  dynamic native execution paradigm”.

## Story structure

1. **Cold open:** a terminal result, failure, or apparently unreasonable goal.
2. **The constraint:** what the machine cannot yet do.
3. **The smallest useful step:** the design decision that keeps the experiment
   understandable.
4. **The experiment:** code, diagram, or terminal evidence.
5. **The reveal:** what actually ran, and at which layer.
6. **The honest boundary:** what has *not* been achieved.
7. **The next door:** one sentence that leads naturally to the next chapter.

## Artwork system

- Editorial hero: charcoal field-journal backdrop, matte paper texture, fine
  drafting lines, warm amber and muted teal accents.
- New publication diagrams: warm paper background, dark text, blue runtime,
  amber execution, violet Lisp tools/generated code, teal enforced boundaries,
  and coral current limitations. Use labels as well as colour. Dashed outlines
  and explicit “planned” labels distinguish future components.
- Keep normal explanations in flowing prose. Diagrams should show architecture,
  containment, topology, or transformations; avoid illustrated prose cards.
- Use the shared `tools/blog/diagramkit.py` components. Prefer one question per
  figure, short labels, and readable text at article width. Existing dark
  figures can migrate article by article; terminal captures retain their palette.
- Use actual identifiers and addresses only when they are verified.
- Terminal: render real transcripts in the same palette; include the command
  and enough output to establish the claim.
- Never generate pseudo-code inside artwork. Real code belongs in typeset
  snippets or reproducible terminal captures.
- Every image requires a caption that says what the reader should notice.

## Platform-neutral front matter

Drafts should keep publisher metadata in an HTML comment:

```markdown
<!--
title: ...
subtitle: ...
status: draft
hero: ../assets/artwork/...
-->
```

This remains invisible in most Markdown renderers and can be translated into
Medium, a static-site generator, or another publishing system later.
