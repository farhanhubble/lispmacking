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

Milestones 0 through 3 are implemented:

- RISC-V cross-toolchain and QEMU environment validated;
- bare-metal RV64 startup, stack initialization, `.bss` clearing, and UART;
- hosted reader, evaluator, printer, and REPL;
- the same evaluator running interactively on bare-metal RISC-V in QEMU;
- automated hosted and QEMU integration tests;
- source-level debugging through QEMU's GDB stub.

An example QEMU session:

```text
Lisp machine booted.
lisp> (cons 'a '(b c))
(a b c)
lisp> (((lambda (x) (lambda (y) (cons x (cons y nil)))) 'a) 'b)
(a b)
lisp>
```

## Language subset

The current language includes:

- symbols and the `nil` singleton;
- cons cells, proper lists, and dotted pairs;
- `quote` and `'` reader syntax;
- `atom`, `eq`, `car`, `cdr`, and `cons`;
- `cond`;
- lexically scoped `lambda`, closures, and application.

Symbols are case-sensitive. The REPL accepts one expression per line.

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

## Testing

```sh
make test        # hosted reader/evaluator/printer tests
make test-qemu   # cross-build and execute expressions on emulated RV64
```

The QEMU test checks normal evaluation, lexical closure capture, type-error
reporting, recovery after an error, and controlled shutdown.

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
platform/hosted/      standard C allocation and terminal adapter
platform/riscv64/     QEMU UART, bump allocator, and shutdown adapter
vm/                   portable object model, reader, evaluator, and printer
tests/                hosted VM tests
docs/                 architecture and design documentation
```

## Current limitations

- Allocation is a bump arena; objects are not reclaimed until restart.
- The bare-metal input line is limited to 511 characters.
- The evaluator is recursive and uses a fixed 16 KiB stack in QEMU.
- UART I/O is polling-based rather than interrupt-driven.
- Bare-metal code currently runs entirely in RISC-V machine mode.
- There are no integers, machine operations, compiler, isolation, IPC, or
  capabilities yet.

## Roadmap

The next planned stages are:

1. machine-oriented values and carefully scoped systems operations;
2. a Lisp-written RISC-V assembler and restricted native compiler;
3. page tables, privilege transitions, traps, scheduling, IPC, and
   capabilities;
4. isolated user-space services and VirtIO drivers.

The full experimental roadmap is recorded in
[CODEX_LISP_MICROKERNEL_BRIEF.md](CODEX_LISP_MICROKERNEL_BRIEF.md).

## License

This project is licensed under the [Mozilla Public License 2.0](LICENSE).

MPL 2.0 is a file-level copyleft license: modifications to MPL-covered files
must remain available under MPL 2.0 when distributed, while separate files may
be combined with them in a larger work under different terms.
