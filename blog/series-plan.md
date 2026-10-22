# Series plan: building a Lisp machine one boundary at a time

The series is a development diary with a technical spine. It begins with the
question about SectorLISP that connected bare-metal programming and Lisp, then
follows the addition of language, native code, isolation, and services.
Articles 1–4 describe implemented and reproducible work. Articles 5–8 are
written from the boundary between what exists and what is planned.

| # | Working title | Technical reveal | Status | Primary evidence |
|---|---|---|---|---|
| Prologue | Bare Metal Enlightenment | A tiny evaluator connects physical startup to a symbolic world that can help construct the machine around itself | Draft | SectorLISP sources; current Lispmacking architecture and demonstrations |
| 1 | It from Bit | Bare-metal startup is a chain of mundane obligations before it is a “machine” | Draft | Boot-path diagram; QEMU boot capture |
| 2 | One Lisp, Two Universes | The same evaluator can live inside an OS process and on a machine with no OS | Draft | Hosted/bare-metal diagram; paired terminal captures |
| 3 | The Privilege of Knowing an Address | Integer and memory primitives become dangerous at the boundary, not in arithmetic itself | Planned | Bootstrap boundary diagram; rejection cases |
| 4 | The Day Lisp Wrote a RISC-V Instruction | Lisp interpreted by C can emit a native function that later runs on the CPU | Draft | Compiler pipeline; native QEMU proof |
| 5 | This Is Not a Microkernel Yet | Running on bare metal and running native code do not provide isolation | Planned | Current-vs-M6 privilege diagram |
| 6 | Crossing the Privilege Boundary | Page tables and traps turn conventions into enforced boundaries | M6 evidence available; writing pending | Trap timeline; page-table walk |
| 7 | Two Worlds, One Endpoint | Capability-scoped IPC can connect isolated Lisp worlds without ambient authority | M6 evidence available; writing pending | Capability graph; two-world demo |
| 8 | The Driver Does Not Belong in the Kernel | A VirtIO service can own device policy outside the privileged core | M7 evidence available; writing pending | VirtIO request path; service terminal trace |

## Season-level arc

The prologue records the question that created the project and states its
destination without pretending we have arrived. Articles 1–2 establish the
two environments. Article 3 introduces authority. Article 4 delivers the first
native bootstrap. Article 5 corrects the tempting but inaccurate conclusion
that we already have a microkernel. Articles 6–8 then build the boundaries and
services that justify the name.

## Definition of publishable

An article is ready when:

- every command shown has been run from a clean checkout;
- every terminal image has a matching plain-text transcript;
- diagrams use only implemented behaviour, except when clearly labelled
  “planned”;
- the title promises the same reveal the article actually delivers;
- the honest-boundary section names at least one limitation;
- the final read is below ten minutes unless the material genuinely requires a
  two-part article;
- links and image alt text work outside Medium.
