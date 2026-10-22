# M6: isolated Lisp worlds

`make test-qemu-m6` boots a separate RV64 image that runs two copies of the
portable Lisp evaluator in user mode. They have separate physical memory and
Sv39 page tables, and exchange integers through explicitly granted endpoints.
The image also runs denial and fault-containment tests, then exits QEMU.

```sh
make microkernel        # build build/microkernel/kernel.elf
make run-microkernel    # watch the demonstrations on the serial console
make test-qemu-m6       # bounded acceptance runner; saves acceptance.log
make test test-qemu     # hosted tests and M4, M5, M6 QEMU tests
```

The prerequisites remain `riscv64-elf-gcc`, matching binutils, QEMU, and Python 3.
`CROSS` and `QEMU` can select alternative tool locations. M6 targets one hart,
128 MiB RAM and the QEMU `virt` memory map with `-bios none`. It does not discover
hardware through the device tree or use OpenSBI.

## Execution boundary

```text
M mode: boot / PMP setup / machine timer relay
                         |
                         v
S mode: Sv39 mappings / trap handling / scheduler / capability tables / IPC
                         |
                sret     |     ecall / fault / timer
                         v
U mode:  Lisp world A                         Lisp world B
         C evaluator                         C evaluator
         private heap and stack              private heap and stack
         send handle 1 ---- endpoint 0 ----> receive handle 1
      receive handle 2 <--- endpoint 1 ----- send handle 2
```

`microkernel/start.S` establishes the machine stack, clears BSS, configures PMP,
and enters S mode. PMP permits the supervisor to manage physical memory; Sv39
restricts the user worlds. User exceptions and supervisor software interrupts
are delegated to S mode. Machine timer interrupts remain in M mode: a small
assembly handler rearms CLINT `mtimecmp` and raises the supervisor software
interrupt. The timer interval is 100,000 ticks (10 ms with QEMU virt's 10 MHz
clock). Counter access is not granted to U mode.

`microkernel/trap.S` swaps to a trusted supervisor stack through `sscratch`,
saves all integer registers and the PC, and invokes the scheduler. A trap while
handling a trap fails closed. Return fixes the privilege bits rather than
trusting saved user state: U mode, 64-bit XLEN, SUM/MXR disabled. The build uses
RV64IMAC, so no floating-point/vector context is enabled or needs saving.

## Address spaces

Both worlds see the same virtual layout but receive distinct physical backing:

| Virtual range | Mapping in each world |
|---|---|
| `0x400000` to linked `__user_ro_end` | User read/execute; native VM text and constants |
| `__user_ro_end` to `0x5ef000` | User read/write, non-executable; globals and bump heap |
| `0x5ef000`–`0x5f0000` | Unmapped stack guard |
| `0x5f0000`–`0x600000` | User read/write, non-executable; 64 KiB stack |
| Kernel RAM identity mapping | Supervisor only; no user access |
| UART and QEMU shutdown device | Supervisor-only MMIO mappings |

Each world has 2 MiB of reserved physical storage, including its unmapped guard
page. All backing storage is cleared before loading the user image. Even code
is privately copied in this initial version. Page-table A/D bits are set in
advance. Every address-space switch writes `satp` and flushes the TLB; ASIDs
are not used. The kernel accesses backing storage through its supervisor
identity mapping and never enables SUM.

The user linker exports the code/data boundary; the build extracts it from the
ELF instead of duplicating a guessed code size in the kernel. User code has no
writable executable mapping. The supervisor identity map itself is broad and
RWX: kernel W^X hardening and narrower kernel mappings remain future work.

The M4/M5 `bootstrap:` operations are unavailable in these U-mode adapters.
The original machine-mode REPL and native compiler remain available through
`make run` and their existing tests. M6 does not yet move the native compiler's
code-page allocation/execution facility across the protection boundary.

## Threads and IPC

There is one thread per world. Threads are runnable, blocked sending, blocked
receiving, exited, or faulted. A timer interrupt or yield chooses the next
runnable thread in round-robin order. Nonblocking syscalls resume their caller;
a blocked or retired caller cannot run. Kernel code is non-preemptible in S
mode, while the M-mode timer relay can still run. With no runnable threads the
current demonstration reports deadlock instead of waiting indefinitely.

Capabilities are entries in supervisor-owned, per-world tables. User programs
supply local slot numbers, never object pointers or global endpoint IDs. Each
entry contains a fixed right and endpoint object. A guessed slot cannot create
a grant, change a right, or index another world's table. Boot policy allows
one sender and one receiver per endpoint and rejects duplicate grants.

The test policy grants each world console output at slot 0 and opposite rights
on two directed endpoints at slots 1 and 2. Slot 3 is empty. Console output is
a bootstrap diagnostic syscall; moving console policy to a user service is M7.

The syscall ABI in `microkernel/abi.h` is:

| `a7` | Operation | Inputs | Result |
|---|---|---|---|
| 0 | exit | `a0`: exit status | does not return |
| 1 | yield | none | `a0`: 0 |
| 2 | send | `a0`: send capability; `a1`: 64-bit value | `a0`: status |
| 3 | receive | `a0`: receive capability | `a0`: status; `a1`: received value |
| 4 | put character | `a0`: console capability; `a1`: byte | `a0`: status |

Statuses are 0 (success), -1 (missing capability or wrong right), -2 (peer gone),
and -3 (unknown syscall). Values use the full 64-bit payload range; the status
is separate. No syscall accepts a user pointer or copies a user buffer.

IPC is synchronous rendezvous. The first participant blocks; the second
transfers the word and wakes it. Pending sends live in kernel endpoint state.
When a participant exits or faults, its endpoints close and any peer blocked
on them wakes with `IPC_PEER_GONE`. Subsequent operations on the closed endpoint
also fail. There is no capability minting, transfer, dynamic revocation,
endpoint creation, or thread creation interface in this milestone.

The user adapter exposes `(ipc:send handle value)` and `(ipc:receive handle)`
to the unchanged VM. Bad arguments or denied operations produce recoverable
Lisp errors. The programs under `tests/microkernel/` define private Lisp values,
exchange 42 and 43, and verify their results. Native private globals at identical
virtual addresses are also checked for independent values.

## Acceptance evidence

`tests/microkernel/run.py` gives QEMU 30 seconds, requires a successful exit and
the exact ordered scenario results, and saves
`build/microkernel/acceptance.log`. The harness checks both Lisp results and
13 scenarios:

- isolated Lisp evaluation and bidirectional IPC, invalid handles, wrong rights,
  unknown syscalls, yield, and full-width payloads;
- timer preemption with the interrupted PC inside a loop that never yields;
- denied reads of kernel memory, known peer physical memory, and UART MMIO;
- denied writes to user text and execution of writable data;
- denied supervisor CSR access, a stack guard fault, and denied kernel execution;
- peer exit waking a blocked receiver and peer fault waking a blocked sender;
- absence of endpoint grants denying communication while both VMs still work.

Page-fault tests check the cause and exact deliberately forbidden address. The
CSR probe requires an illegal-instruction trap. After each fault, the surviving
world must successfully evaluate Lisp and exit. The test intentionally gives
one world a peer physical address to demonstrate that knowing an address does
not grant access to its backing memory.

The kernel mechanisms live in `kernel.c` and the assembly files. Static grants
and test progression live in `demo.c`; user probes and VM integration live in
`user.c`. The bootable target is an acceptance demonstrator, not an interactive
multi-user OS. Kernel assertions and test orchestration remain in the trusted
image. The runtime still uses a bump allocator, and only the QEMU configuration
above is validated.

Architecture references: [RISC-V supervisor specification](https://docs.riscv.org/reference/isa/v20260120/priv/supervisor.html),
[RISC-V machine specification](https://docs.riscv.org/reference/isa/v20260120/priv/machine.html),
and [QEMU virt platform](https://www.qemu.org/docs/master/system/riscv/virt.html).

## Following milestone

M7 now provides a separate three-world service target. The M6 demonstration
and its 13 scenarios remain available unchanged. See [User-space services](services.md).
