# Experimental Lisp Microkernel

Milestones 1 through 3 provide two execution environments for the same small
Lisp evaluator:

- a hosted macOS REPL for fast tests and debugging;
- an RV64 bare-metal serial REPL running on QEMU's `virt` machine.

## Prerequisites

Install the required tools with Homebrew:

```sh
brew install qemu riscv64-elf-gcc riscv64-elf-gdb
```

`riscv64-elf-gcc` installs the matching binutils as a dependency.

## Build and run on RISC-V QEMU

```sh
make
make run
```

The serial console is interactive. For example:

```text
Lisp machine booted.
lisp> (cons 'a '(b c))
(a b c)
lisp>
```

The REPL reads one expression per line and echoes UART input. Press Control-D
at the prompt to exit QEMU. Run a reproducible scripted serial test with:

```sh
make test-qemu
```

Build products are written under `build/`. Remove them with `make clean`.

## Hosted Lisp

Build the hosted interpreter or run its tests with the system C compiler:

```sh
make hosted
make test
make repl
```

The REPL reads one expression per line. End it with Control-D. Examples:

```lisp
(cons 'a '(b c))
((lambda (x) (cons x '(b))) 'a)
(cond ((eq 'a 'b) 'wrong) (t 'right))
```

The language contains symbols, `nil`, cons cells, `quote` (and `'`
syntax), `atom`, `eq`, `car`, `cdr`, `cons`, `cond`, lexical `lambda`, and
application. Lists and dotted pairs can be read and printed. Symbols are
case-sensitive.

The QEMU adapter uses polling UART I/O and a 16-byte-aligned bump allocator
over the remaining explicitly configured 128 MiB of emulated RAM. Individual
allocations are not reclaimed yet; the arena resets when the machine restarts.
A collector is deferred until measured allocation pressure justifies it.

### VM invariants

- Every Lisp value is a stable pointer to a tagged `VmObject`.
- `nil` is a unique object; it is not C's null pointer.
- Symbols are interned, so `eq` can use pointer identity.
- Environments are immutable lists of `(symbol . value)` bindings.
- A lambda closes over the environment in which it was evaluated.
- All allocation passes through `VmPlatform`; hosted allocation is the only
  code that currently calls `malloc` and `free`.
- Objects live until `vm_destroy`. Garbage collection is intentionally deferred
  until allocation pressure requires it in a later milestone.

## Debug

In one terminal, start QEMU paused with its GDB server on TCP port 1234:

```sh
make debug
```

In another terminal, connect GDB:

```sh
riscv64-elf-gdb build/kernel.elf -ex 'target remote :1234'
```

Use `continue` to run. QEMU enters the Lisp prompt; press Control-D in its
terminal to exit, then quit GDB separately if needed.
