# Project roadmap

This document defines the technical milestones for growing the current Lisp
runtime into an experimental capability-oriented microkernel. GitHub issues
track individual deliverables; GitHub milestones group those issues and report
execution progress. This file remains the source of truth for the intent and
acceptance criteria of each stage.

The milestones are deliberately sequential. Later work may be explored in
parallel, but a milestone is complete only when its demonstration and tests run
on the RV64 QEMU target.

## Completed foundations

### Milestone 0 — Validate the development environment

Status: complete.

The RISC-V cross-toolchain, binutils, debugger, and QEMU environment were
validated, including successful launch of `qemu-system-riscv64`.

### Milestone 1 — Boot and print

Status: complete.

The RV64 image starts at a defined entry point, establishes its stack, clears
`.bss`, writes through the QEMU `virt` UART, and supports QEMU's GDB stub.

Demonstration: `make run` prints `Lisp machine booted.`

### Milestone 2 — Hosted minimal Lisp

Status: complete.

The hosted runtime provides symbols, cons cells, `nil`, quotation, the core
list primitives, `cond`, lexical closures, application, a reader, a printer,
and a REPL. The VM is portable C11 behind a small platform interface.

Demonstration: `make test` passes the hosted VM test suite and `make repl`
starts an interactive native process.

### Milestone 3 — Bare-metal evaluator

Status: complete.

The same VM sources run on bare-metal RV64 with a UART console and arena
allocator. Platform-specific allocation, input, output, and shutdown behavior
remain outside the evaluator core.

Demonstration: `make test-qemu` cross-compiles the ELF, boots it under QEMU,
evaluates representative expressions, checks error recovery, and exits
cleanly.

## Completed machine substrate

### Milestone 4 — Machine-oriented Lisp

Status: complete.

Add values and operations needed for systems programming without turning every
Lisp expression into an unrestricted machine-mode operation.

Scope:

- define fixed-width integer representation, parsing, printing, arithmetic,
  comparison, and overflow behavior;
- define an explicit boundary between ordinary language forms and privileged
  bootstrap operations;
- add carefully scoped memory access and page-allocation operations;
- add the minimum CSR and interrupt-control operations required by the next
  milestone;
- preserve the shared evaluator core and make unsupported privileged
  operations fail clearly in the hosted environment;
- test language semantics on the host and privileged behavior on RV64 QEMU.

Acceptance criteria:

- integer literals round-trip through the reader and printer;
- documented arithmetic and comparison operations pass hosted and QEMU tests;
- invalid types, invalid addresses, alignment violations, and unavailable
  privileged operations produce recoverable errors;
- privileged operations are visibly distinct in the API and documented with
  their invariants;
- page allocation returns aligned, non-overlapping memory from a documented
  region;
- a QEMU demonstration performs a bounded machine-oriented operation and then
  returns safely to the REPL;
- `make test` and `make test-qemu` remain green.

Demonstration: `make test-qemu` performs checked arithmetic, allocates distinct
pages, stores and reloads a value through bounded memory access, validates
alignment and range failures, round-trips the allowlisted `mscratch` CSR, and
restores interrupt state before returning safely to the REPL.

## Completed native bootstrap

### Milestone 5 — Lisp-written assembler and compiler

Status: complete.

Implement a small RISC-V assembler or code emitter in Lisp, followed by a
restricted native compiler supporting integers, calls, branches, locals, and
explicit memory operations.

Acceptance demonstration: compile a Lisp function to native RISC-V code,
execute it under QEMU, and verify its result against the evaluator.

The completed compiler uses Lisp-written R/I/S/B/J encoders, a documented
one-argument RV64 ABI, fixed function slots, PC-relative branch/call patching,
and an explicitly allocated code page. `make test-qemu` loads the Lisp sources,
compiles four functions, executes generated calls, branches, and memory
stores, and checks native results against the evaluator.

## Planned milestones

### Milestone 6 — Microkernel boundary

Introduce page tables, privilege transitions, traps, timers, threads, IPC, and
capabilities. Keep policy out of the privileged core wherever practical.

Acceptance demonstration: run two isolated language worlds that communicate
only through an explicitly granted endpoint.

### Milestone 7 — User-space services

Move useful facilities into isolated services, beginning with the console and
a simple VirtIO device. Prefer portable VirtIO interfaces over legacy
hardware-specific devices.

Acceptance demonstration: a user-space service handles a real VirtIO-backed
operation without adding that device policy to the microkernel.

## Tracking conventions

- One GitHub milestone corresponds to one technical milestone in this file.
- An issue represents one independently reviewable and testable deliverable.
- Every implementation issue states its acceptance criteria and expected test
  environment.
- Pull requests link their issue and update this document when milestone scope
  or acceptance criteria change.
- Completed milestones remain here as project history; they do not need
  retrospective placeholder issues.
