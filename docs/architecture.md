# Architecture

The project uses one portable Lisp evaluator with small platform adapters. The
goal is to make language semantics easy to test on a development machine while
ensuring that the same implementation can run without an operating system.

```text
                         Portable Lisp VM
                  object / reader / evaluator / printer
                            /           \
                           /             \
              Hosted adapter          RISC-V adapter
              malloc + stdio           arena + UART
                    |                       |
             Host OS process         QEMU virt machine
```

## Execution environments

| | Hosted environment | Bare-metal environment |
|---|---|---|
| Runs on | A normal host operating system | QEMU's emulated RV64 CPU |
| Binary | Native host executable | RISC-V bare-metal ELF |
| Compiler | Host `cc` | `riscv64-elf-gcc` |
| Memory | `malloc` and `free` | Fixed bump allocator over emulated RAM |
| Input/output | Standard C `stdio` | QEMU `virt` UART registers |
| Startup | Operating system launches `main` | Assembly establishes the stack and clears `.bss` |
| C library | Available to the adapter | None |
| Process services | Supplied by the host OS | No underlying operating system |
| Exit | Return from `main` | Write a pass code to QEMU's test device |

The two builds are not the same executable. They compile the same VM sources
against different entry points and platform adapters.

## Portable VM

The shared implementation lives under `vm/`:

- `object.c` implements tagged objects, cons cells, symbol interning, and
  environments;
- `reader.c` converts source text into Lisp objects;
- `evaluator.c` implements special forms, primitives, closures, and
  application;
- `printer.c` writes Lisp objects through a callback;
- `vm.h` defines the public object and platform interfaces.

The VM receives allocation functions through `VmPlatform` and emits bytes
through a `VmWrite` callback. It does not include `stdio.h`, call `malloc`, or
access hardware registers directly.

Important invariants:

- every Lisp value is a stable pointer to a tagged `VmObject`;
- Lisp `nil` is a unique object, not a C null pointer;
- symbols are interned, allowing `eq` to use pointer identity;
- environments are immutable lists of `(symbol . value)` bindings;
- lambdas retain the environment in which they were evaluated;
- objects currently live until the VM is destroyed or the machine restarts.

## Hosted path

The hosted adapter under `platform/hosted/` uses standard C allocation and
I/O. The operating system loads the native program, provides its address space,
and launches `main`. This path is intended for rapid tests, sanitizers,
fuzzing, and debugger use.

The adapter has no macOS-specific code. The project was initially verified on
macOS, but the C sources are intended to be portable. The current Makefile and
test commands assume a Unix-like shell environment.

## Bare-metal path

The RISC-V build starts at `_start` in `boot/start.S`:

```text
QEMU reset vector
    -> _start
    -> establish stack
    -> clear .bss
    -> kernel_main
    -> initialize arena and VM
    -> serial Lisp REPL
```

The linker places the image at `0x80000000`, the base of RAM on QEMU's `virt`
machine. The platform adapter under `platform/riscv64/` provides:

- polling input and output through the memory-mapped 16550-compatible UART;
- a 16-byte-aligned bump allocator over unused emulated RAM;
- line editing for backspace and a bounded input buffer;
- controlled shutdown through QEMU's test device.

The build is freestanding: it uses no host operating system and links no C
library. QEMU is explicitly configured with 128 MiB of RAM to match the linker
map.

## Portability boundaries

The language core is not tied to macOS. Porting the hosted build generally
means supplying a C11 toolchain and compatible build commands. Porting the
bare-metal build to another board or ISA requires a new startup path, linker
map, allocator, and console adapter, while the VM sources should remain shared.

The current bare-metal adapter is specifically tied to:

- RISC-V RV64IMAC;
- QEMU's `virt` memory map;
- its 16550-compatible UART and test device.

Those are adapter constraints rather than Lisp-language constraints.
