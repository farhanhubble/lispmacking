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

## Completed microkernel boundary

### Milestone 6 — Microkernel boundary

Status: complete for the bounded QEMU acceptance target.

The separate `microkernel/` image enters S mode and runs two copies of the
portable evaluator in U mode with private Sv39 address spaces. It implements
full integer trap frames, timer-driven round-robin scheduling, fixed per-world
capability tables, synchronous scalar IPC, and peer-fault/exit cleanup.

Acceptance demonstration: `make test-qemu-m6` runs two isolated language worlds
that exchange values only through explicitly granted endpoints. It also checks
13 scenarios including missing/wrong capabilities, timer preemption inside a
non-yielding loop, hardware-enforced memory/CSR denials, and peer survival.
`make test` and the existing M4/M5 QEMU tests remain green.

This milestone is deliberately bounded: two static worlds, one thread each,
boot-time grants, no capability transfer, and no dynamic user code generation.
The M4/M5 machine-mode laboratory remains a separate target. Console output is
still a privileged diagnostic facility. See [Microkernel boundary](microkernel.md)
for the ABI, memory layout, evidence, and remaining limitations.

## Planned milestones

### Milestone 7 — User-space services

Move useful facilities into isolated services, beginning with the console and
a simple VirtIO device. Prefer portable VirtIO interfaces over legacy
hardware-specific devices.

Acceptance demonstration: a user-space service handles a real VirtIO-backed
operation without adding that device policy to the microkernel.

### M7 implementation tracking

Status: planned; implementation has not started.

- [M7: Define service requests, completions, and buffer ownership](https://github.com/farhanhubble/lispmacking/issues/24)
- [M7: Move console output into an isolated service](https://github.com/farhanhubble/lispmacking/issues/25)
- [M7: Implement an isolated VirtIO block service](https://github.com/farhanhubble/lispmacking/issues/26)
- [M7: Add end-to-end service acceptance and documentation](https://github.com/farhanhubble/lispmacking/issues/27)

The proposed integration demonstration reads Lisp source through a block service,
evaluates it in an isolated world, and prints through the console service.
Service protocols should leave room for batching and asynchronous completions,
with explicit buffer ownership and separately documented DMA authority.

Multicore support is not an M7 acceptance requirement. The discussion of IPC
latency, throughput, CPU cost, same-core handoffs, and cross-core overlap is
tracked as a [future performance investigation](https://github.com/farhanhubble/lispmacking/issues/28). QEMU establishes correctness; claims about hardware performance need
measurements on the target hardware.

## Tracking conventions

- One GitHub milestone corresponds to one technical milestone in this file.
- An issue represents one independently reviewable and testable deliverable.
- Every implementation issue states its acceptance criteria and expected test
  environment.
- Pull requests link their issue and update this document when milestone scope
  or acceptance criteria change.
- Completed milestones remain here as project history; they do not need
  retrospective placeholder issues.
