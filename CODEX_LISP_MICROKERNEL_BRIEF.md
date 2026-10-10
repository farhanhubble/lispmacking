# Experimental Lisp Microkernel — Codex Project Brief

## Purpose

We want to explore whether a small Lisp-like language runtime can grow, through progressive bootstrapping, into a capability-oriented microkernel operating system.

The long-term idea is a system in which:

- a tiny language machine boots directly on hardware;
- that machine loads or constructs progressively richer evaluators and compilers;
- the lowest trusted layer provides isolation, scheduling, interrupts, memory management, IPC, and capability enforcement;
- drivers and operating-system facilities run as isolated language services;
- hardware features such as SIMD, GPUs, tensor units, and FPGAs can be exposed through compiler/runtime plugins rather than permanently built into the core;
- higher-level processes can create their own small language worlds or application-specific kernels.

This is an experiment, not an attempt to build a production OS immediately. Optimize initially for clarity, observability, small trusted components, and rapid iteration.

## Local machine

The development host is a MacBook Pro with Homebrew. Do not assume whether it is Intel or Apple Silicon. Inspect the host before choosing architecture-specific flags:

```bash
uname -m
sw_vers -productVersion
brew --version
xcode-select -p
```

Keep every project file inside the folder opened in Codex. Do not modify unrelated files or global shell configuration. Ask before performing privileged, destructive, or unusually broad system changes.

## Initial technical direction

Use these defaults unless inspection reveals a concrete blocker:

- Target ISA: RISC-V 64-bit.
- Emulator: QEMU `riscv64`, `virt` machine.
- Initial boot method: direct ELF loading with `-bios none`.
- Initial interface: UART serial console with `-nographic`.
- Cross toolchain: `riscv64-elf-gcc` and its binutils.
- Debugger: `riscv64-elf-gdb` through QEMU's GDB stub.
- Bootstrap implementation: minimal RISC-V assembly plus freestanding C.
- Higher layers: progressively implemented in the experimental Lisp.
- Build system: a small, readable Makefile unless there is a strong reason otherwise.

Expected Homebrew packages:

```bash
brew install qemu riscv64-elf-gcc riscv64-elf-gdb
```

Before installing anything, check whether each command is already available. Do not upgrade all Homebrew packages as part of this task.

## Architectural principles

### 1. Maintain a portable VM core

The evaluator/runtime should eventually run through two adapters:

1. A hosted adapter running as a normal macOS process for unit tests, fuzzing, and fast debugging.
2. A bare-metal adapter running inside QEMU.

Keep platform-specific I/O and memory operations outside the portable evaluator.

Conceptually:

```text
Portable Lisp VM
├── hosted macOS adapter
└── bare-metal RISC-V adapter
```

### 2. Bootstrap rather than indefinitely nest interpreters

The intended progression is:

```text
small native substrate
    -> Lisp evaluator
    -> Lisp-written assembler/compiler
    -> native compiled Lisp
    -> isolated Lisp service worlds
```

Earlier stages should become scaffolding, recovery tools, or specifications. Avoid retaining many interpretive layers when a later layer can compile away the earlier overhead.

### 3. Separate mechanism from policy

The eventual microkernel should enforce only essential mechanisms:

- address-space isolation;
- privilege transitions;
- traps and interrupts;
- scheduling primitives;
- IPC endpoints;
- capability ownership and transfer;
- resource accounting.

Filesystems, networking, device policies, graphical services, and most drivers should eventually live outside the trusted kernel.

### 4. Treat hardware extensions as language/runtime plugins

Do not add SIMD or CUDA during the initial milestones, but keep the architecture open to plugins that may provide:

- language forms and types;
- macro expansion;
- compiler lowering and instruction selection;
- runtime services;
- hardware discovery;
- capability and quota policy;
- scalar or host-side fallbacks.

SIMD can later be a CPU compiler backend extension. CUDA should initially be represented as an external host-side GPU service reached through an explicit transport; do not attempt an NVIDIA kernel driver or GPU passthrough in the early project.

### 5. Keep privileged operations capability-scoped

Early bootstrap code may temporarily expose raw memory operations. The eventual model must not give ordinary language worlds unrestricted physical memory or I/O access. Device access should require explicit, unforgeable capabilities.

### 6. Prefer comprehensibility over byte-count records

SectorLISP and sectorFORTH are inspiration, not size targets. The first implementation may be tens of kilobytes if that keeps it inspectable, testable, and easy to evolve.

## Proposed repository structure

Do not create every empty directory pre-emptively. This is the intended direction as components appear:

```text
.
├── boot/
│   ├── start.S
│   └── linker.ld
├── kernel/
│   ├── uart.c
│   ├── panic.c
│   ├── memory.c
│   ├── traps.c
│   ├── scheduler.c
│   └── capabilities.c
├── vm/
│   ├── object.c
│   ├── reader.c
│   ├── evaluator.c
│   └── gc.c
├── platform/
│   ├── hosted/
│   └── riscv64/
├── lisp/
│   ├── assembler.lisp
│   ├── compiler.lisp
│   └── services/
├── tests/
├── Makefile
└── README.md
```

## Roadmap

### Milestone 0 — Validate the local environment

- Inspect CPU architecture, macOS version, Homebrew, and developer tools.
- Verify or install QEMU, RISC-V GCC, binutils, and RISC-V GDB.
- Record tool versions in the task report, not in source files unless reproducibility later requires it.
- Confirm that `qemu-system-riscv64` launches.

### Milestone 1 — Boot and print

Create the smallest clear bare-metal program that:

- starts at a defined RISC-V entry point;
- establishes a valid stack;
- clears `.bss` if required;
- writes through the QEMU `virt` UART;
- prints exactly one boot message;
- exits QEMU automatically if practical, or enters a documented halt loop.

Target output:

```text
Lisp machine booted.
```

Provide convenient commands such as:

```bash
make
make run
make debug
```

`make debug` should start QEMU paused with a GDB server, normally using `-S -s`.

### Milestone 2 — Hosted minimal Lisp

Build and test a portable evaluator as a normal host process before embedding it bare-metal. Begin with a deliberately small language:

- atoms/symbols;
- cons cells and `nil`;
- `quote`;
- `atom`, `eq`, `car`, `cdr`, and `cons`;
- `cond`;
- lexical environment representation;
- `lambda` and application;
- reader and printer sufficient for a REPL.

Do not optimize prematurely. Define the object representation explicitly and document important invariants.

### Milestone 3 — Bare-metal evaluator

- Reuse the same evaluator core in QEMU.
- Replace host allocation and I/O with platform adapters.
- Add a basic allocator and then a simple collector when allocation pressure requires it.
- Provide a serial REPL.

### Milestone 4 — Machine-oriented Lisp

Add explicit fixed-width integers and carefully scoped operations needed for systems work, such as memory access, page allocation, CSR access, and interrupt-safe functions. Clearly distinguish privileged bootstrap forms from ordinary user-world forms.

### Milestone 5 — Lisp-written assembler/compiler

Implement a small RISC-V assembler or code emitter in Lisp, followed by a restricted native compiler. Initially support only integers, calls, branches, locals, and explicit memory operations. Demonstrate that a Lisp function can compile to native code and execute correctly.

### Milestone 6 — Microkernel boundary

Add page tables, user/supervisor transitions, traps, timers, threads, IPC, and capabilities. The milestone demonstration should be two isolated language worlds communicating through an explicitly granted endpoint.

### Milestone 7 — User-space services

Add services progressively, starting with console and a simple VirtIO device. Prefer VirtIO block/network devices over hardware-specific legacy devices.

## Immediate task for Codex

Work only on **Milestones 0 and 1** initially.

1. Inspect the Mac and summarize what is already installed.
2. Install only missing required Homebrew packages, subject to normal approval controls.
3. Create a minimal Git repository if the folder is not already a repository.
4. Implement the RISC-V boot program, linker script, UART output, and Makefile.
5. Run it in QEMU and capture the actual serial output.
6. Verify `make`, `make run`, and the debug launch path.
7. Add a concise README explaining prerequisites, build, run, and debug commands.
8. Stop after Milestone 1 and report:
   - files created or changed;
   - commands run;
   - observed boot output;
   - any deviations from this brief;
   - recommended next step for Milestone 2.

Do not begin implementing the Lisp evaluator until Milestone 1 has been demonstrated successfully and reviewed.

## Milestone 1 acceptance criteria

The work is complete only when:

- the repository builds from a clean state using the documented command;
- `build/kernel.elf` is a RISC-V bare-metal ELF;
- QEMU boots it using the documented invocation;
- the serial console visibly contains `Lisp machine booted.`;
- build artifacts are ignored by Git;
- no project dependency is hidden in an undocumented global shell modification;
- the code and linker script are small enough to inspect directly;
- failure modes return useful errors where the host tooling permits it.

## Working style

- Lead with observed evidence rather than assumptions.
- Keep changes small and reviewable.
- Explain architectural trade-offs when they materially affect future stages.
- Do not silently broaden the task.
- Preserve any existing user changes in the repository.
- Prefer reproducible commands over manual GUI operations.
- Never place credentials or secrets in the repository.
