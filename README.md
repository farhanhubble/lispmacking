# Experimental Lisp Microkernel

An experiment in growing a small Lisp-like runtime into a capability-oriented
microkernel through progressive bootstrapping.

The project currently provides the same Lisp evaluator in two environments:

- a hosted native REPL for fast tests and debugging;
- an RV64 bare-metal serial REPL running on QEMU's `virt` machine.

The evaluator is written in portable C11. Platform-specific allocation and I/O
live behind small adapters, so the language core does not depend on macOS or on
an operating system. See [Architecture](docs/architecture.md) for a detailed
comparison of the two environments.

> [!NOTE]
> This is an exploratory system, not a production operating system. The current
> priorities are clarity, observability, and small trusted components.

## Current status

Milestones 0 through 7 are implemented:

- RISC-V cross-toolchain and QEMU environment validated;
- bare-metal RV64 startup, stack initialization, `.bss` clearing, and UART;
- hosted reader, evaluator, printer, and REPL;
- the same evaluator running interactively on bare-metal RISC-V in QEMU;
- automated hosted and QEMU integration tests;
- signed 64-bit integers with checked arithmetic and comparison;
- a bounded, explicitly privileged bootstrap interface for pages, memory,
  allowlisted CSRs, and interrupt state;
- a Lisp-written RV64 assembler and restricted native compiler;
- validated execution of generated Lisp functions under QEMU;
- source-level debugging through QEMU's GDB stub;
- a separate S-mode microkernel target with two isolated U-mode Lisp worlds,
  timer-driven scheduling, and capability-checked IPC;
- isolated console and VirtIO block services, loading and evaluating Lisp from disk.

An example QEMU session:

```text
Lisp machine booted.
lisp> (cons 'a '(b c))
(a b c)
lisp> (((lambda (x) (lambda (y) (cons x (cons y nil)))) 'a) 'b)
(a b)
lisp>
```

The M6 target is a bounded acceptance demonstrator. It uses the same VM in
isolated user address spaces; `make run` retains the original machine-mode
bootstrap REPL. See [Microkernel boundary](docs/microkernel.md).

## Language subset

The current language includes:

- symbols and the `nil` singleton;
- cons cells, proper lists, and dotted pairs;
- `quote` and `'` reader syntax;
- `atom`, `eq`, `car`, `cdr`, and `cons`;
- `cond`;
- lexically scoped `lambda`, closures, and application;
- signed 64-bit integer literals;
- checked `+`, `-`, `*`, `=`, `<`, `<=`, `>`, and `>=` primitives;
- reusable top-level `define`, recursive functions, and sequential `begin`;
- integer `quotient`, `mod`, and `integer?` primitives used by the assembler.

Symbols are case-sensitive. The REPL accepts one expression per line.
Machine-mode operations are visibly separated under the `bootstrap:` prefix;
see the [machine-oriented interface](docs/machine-interface.md) for their
contract and safety checks.
The [compiler reference](docs/compiler.md) documents the native source subset,
RV64 ABI, instruction emitter, and execution boundary.

## Prerequisites

For the hosted evaluator:

- a C11 compiler available as `cc`;
- POSIX-compatible `make` and shell utilities.

For the bare-metal build:

- `qemu-system-riscv64`;
- `riscv64-elf-gcc` and matching binutils;
- `riscv64-elf-gdb` for source-level debugging.

On macOS these can be installed with Homebrew:

```sh
brew install qemu riscv64-elf-gcc riscv64-elf-gdb
```

Package names vary on other systems. The hosted evaluator itself contains no
macOS-specific APIs. The current Makefile assumes a Unix-like command-line
environment; Linux and BSD should need little or no adjustment, while native
Windows may require WSL, MSYS2, or a small build-system adaptation.

## Quick start

### Hosted REPL

Build and test with the host compiler:

```sh
make test
make repl
```

Press Control-D to leave the REPL.

### RISC-V QEMU REPL

Cross-compile the bare-metal ELF and start QEMU:

```sh
make
make run
```

The QEMU serial console echoes input. Press Control-D at the Lisp prompt to
exit QEMU cleanly.

Run the reproducible end-to-end QEMU test with:

```sh
make test-qemu
```

Build products are placed under `build/`. Remove them with `make clean`.

### Isolated Lisp worlds (M6)

```sh
make run-microkernel    # boot the isolation and IPC demonstrations
make test-qemu-m6       # run all 13 bounded acceptance scenarios
```

M6 builds `build/microkernel/kernel.elf` and embeds a separately linked user
image. Python 3 is required for the build helpers and acceptance runner.

### User-space device services (M7)

```sh
make run-services      # disk -> isolated Lisp evaluator -> console service
make test-qemu-m7       # 11 scenarios and two negative controls
```

See [User-space services](docs/services.md) for the request protocol, device
permissions, DMA trust boundary, and test evidence.

## Testing

```sh
make test        # hosted reader/evaluator/printer tests
make test-qemu   # M4/M5 bootstrap, M6 isolation, M7 device services
```

The QEMU test checks ordinary evaluation, integer overflow, page allocation,
bounded memory access, CSR allowlisting, interrupt-state restoration, and a
separate compiler boot in which Lisp emits and executes native RV64 functions.

## Debugging

Start QEMU paused with a GDB server on TCP port 1234:

```sh
make debug
```

In another terminal:

```sh
riscv64-elf-gdb build/kernel.elf -ex 'target remote :1234'
```

Set a breakpoint such as `break kernel_main`, then use `continue`. QEMU will
enter the Lisp prompt after startup.

## Repository layout

```text
boot/                 RISC-V entry point and linker script
kernel/               bare-metal REPL entry
microkernel/          S-mode kernel, U-mode adapter, boot and trap paths
services/             U-mode console/VirtIO services and M7 acceptance policy
platform/hosted/      standard C allocation and terminal adapter
platform/riscv64/     QEMU UART, bump allocator, and shutdown adapter
vm/                   portable object model, reader, evaluator, and printer
lisp/                 Lisp-written RV64 assembler and restricted compiler
tests/                hosted VM tests
docs/                 architecture and design documentation
blog/                 platform-neutral article drafts and visual assets
tools/blog/           reproducible diagram and terminal-capture tools
```

## Development stories

The [Lispmacking field notes](blog/README.md) tell the development story from
the first UART byte through native Lisp-generated RV64 code and the planned
microkernel boundary. The article sources are publication-neutral Markdown;
diagrams are editable SVGs, and terminal images are rebuilt from real test
transcripts with `make blog-assets`.

## Current limitations

The original M4/M5 bootstrap REPL has these limitations:

- Allocation is a bump arena; objects are not reclaimed until restart.
- The bare-metal input line is limited to 4095 characters.
- The evaluator is recursive and uses a fixed 16 KiB stack in QEMU.
- UART I/O is polling-based rather than interrupt-driven.
- Bare-metal code currently runs entirely in RISC-V machine mode.
- Bootstrap machine operations currently have machine-mode authority; there is
  no user/supervisor isolation, IPC, or capability enforcement yet.
- The native compiler supports only a deliberately restricted one-argument
  integer subset and fixed-size function slots.
- Executable bootstrap pages remain writable and generated code still runs in
  machine mode.

The M6 image adds user isolation but currently has two static worlds, one thread
per world, fixed boot-time capability grants, and scalar rendezvous IPC. It has
no dynamic process creation or capability transfer, and does not yet expose
the native compiler's code-generation interface to user worlds. Its supervisor
mapping is broad and RWX, and console output remains in the kernel.

## Roadmap

M7 is complete for the bounded QEMU target: three static worlds, isolated console
and read-only VirtIO block services, and disk-loaded Lisp evaluation. It uses
synchronous copied IPC and a trusted DMA driver without an IOMMU. Multicore
performance remains an open investigation.

M8–M19 are now planned in [detailed milestone definitions](docs/future-milestones.md),
with matching GitHub milestones. The next work is the interactive workspace (M8)
and ESP32-C3 runtime/portability track (M9). Later milestones preserve separate
steps for systems Lisp, compiler extensions, persistence, managed worlds, the
native C3 kernel, vectors, asynchronous services, multicore and accelerators.

See the [project roadmap](docs/roadmap.md) for milestone definitions, current
status, and acceptance criteria. The original experimental direction is
recorded in [CODEX_LISP_MICROKERNEL_BRIEF.md](CODEX_LISP_MICROKERNEL_BRIEF.md).

## License

This project is licensed under the [Mozilla Public License 2.0](LICENSE).

MPL 2.0 is a file-level copyleft license: modifications to MPL-covered files
must remain available under MPL 2.0 when distributed, while separate files may
be combined with them in a larger work under different terms.
