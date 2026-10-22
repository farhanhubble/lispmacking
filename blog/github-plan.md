# GitHub tracking plan

Editorial work is tracked separately from technical milestones M6 and M7. The
proposed GitHub milestone is **Blog series — Season 1** with the labels
`writing` and `artwork`. Article issues may reference implementation
milestones, but completing an article must never imply that its technical
subject is complete.

## Proposed issues

### Prologue — Bare Metal Enlightenment

- [x] Record the originating ChatGPT question and the missing connection.
- [x] Explain SectorLISP as a bootstrap demonstration rather than a byte-count curiosity.
- [x] Connect the insight to the implemented Lispmacking stack and planned microkernel boundary.
- [x] Replace the conceptual figure with a QEMU architecture diagram and keep explanations in prose.
- [x] Export the architecture as SVG and PNG with caption and alt text.
- [ ] Verify the SectorLISP technical claims against its primary sources.
- [x] Carry the revised introduction’s conversational voice through the prologue.
- [ ] Obtain the author’s final voice review and publish.

### Establish the reproducible blog asset pipeline

- [x] Add platform-neutral Markdown structure.
- [x] Add deterministic SVG diagram generation.
- [x] Capture terminal output from real hosted and QEMU tests.
- [x] Add a publication-quality series hero.
- [x] Add reusable diagram primitives and a read-only SVG freshness command.
- [x] Document 1600-pixel PNG export for the prologue via pinned CairoSVG.
- [ ] Add CI coverage that rebuilds assets and rejects stale output.
- [ ] Document social-card export sizes (article PNG sizing is documented).

### Article 1 — It from Bit

- [x] Produce a substantial first draft.
- [x] Add the boot-path diagram.
- [x] Add a real QEMU terminal capture.
- [ ] Perform technical review against startup assembly and the linker script.
- [ ] Perform final voice edit and publish.

### Article 2 — One Lisp, Two Universes

- [x] Produce a substantial first draft.
- [x] Add the hosted-versus-bare-metal diagram.
- [x] Add a real hosted test capture.
- [ ] Add a short paired REPL demonstration.
- [ ] Perform final voice edit and publish.

### Article 3 — The Privilege of Knowing an Address

- [ ] Draft the story around integers, pages, checked memory and the
  `bootstrap:` namespace.
- [ ] Diagram ordinary evaluation versus privileged platform callbacks.
- [ ] Capture both a successful memory round trip and rejected accesses.
- [ ] Explain what bounds checking does and does not protect.
- [ ] Review and publish.

### Article 4 — The Day Lisp Wrote a RISC-V Instruction

- [x] Produce a substantial first draft.
- [x] Add the compiler-pipeline diagram.
- [x] Add the real QEMU native-compiler transcript.
- [ ] Add one instruction-format close-up using independently verified bits.
- [ ] Perform final voice edit and publish.

### Article 5 — This Is Not a Microkernel Yet

- [ ] Draft the architectural interlude distinguishing bare metal, native
  compilation and isolation.
- [ ] Diagram current machine-mode authority versus the planned M6 boundary.
- [ ] Explain writable executable pages and why validation is not isolation.
- [ ] Review terminology and publish before the first M6 implementation post.

### Article 6 — Crossing the Privilege Boundary

- [x] Hold the article until M6 page tables, supervisor mode and traps have an
  end-to-end QEMU demonstration.
- [ ] Capture a real privilege transition and fault.
- [ ] Diagram the trap path and page permissions.
- [ ] Draft, review and publish from implemented evidence.

### Article 7 — Two Worlds, One Endpoint

- [x] Hold the article until the M6 two-world capability/IPC demonstration.
- [ ] Use the capability graph as the running example.
- [ ] Capture permitted communication and a denied ambient access.
- [ ] Draft, review and publish from implemented evidence.

### Article 8 — The Driver Does Not Belong in the Kernel

- [ ] Hold the article until the M7 user-space VirtIO demonstration.
- [ ] Diagram the request path across the service boundary.
- [ ] Capture the real VirtIO-backed operation.
- [ ] Compare the trusted core before and after moving device policy out.
- [ ] Draft, review and publish from implemented evidence.

## M6 implementation evidence

The implementation prerequisites for Articles 6 and 7 are satisfied by
[710b540](https://github.com/farhanhubble/lispmacking/commit/710b5407b5aacaba619348958af984a41b7110e3).
See [the architecture and acceptance evidence](../docs/microkernel.md).
Writing, article-specific captures, diagrams, review, and publication remain open.
