# M7: user-space console and VirtIO services

`make run-services` boots three isolated U-mode worlds: a Lisp client, a console
service, and a VirtIO block service. The client reads Lisp source from a real
QEMU disk, evaluates it to 42, and prints through the console service. VirtIO
negotiation, queue management, block policy, and normal console output execute
in user space. The supervisor supplies mappings, scheduling, traps and IPC.

```sh
make services          # build kernel, user image and deterministic disk
make run-services      # watch the demonstration and fault scenarios
make test-qemu-m7       # 11 scenarios plus changed-disk/missing-device controls
make test test-qemu     # hosted tests and M4, M5, M6, M7 QEMU suites
```

The target is QEMU `virt`, RV64, one hart, 128 MiB, `-bios none`, and modern
VirtIO MMIO (`virtio-mmio.force-legacy=false`). The sole VirtIO block device is
at `0x10008000` in this fixed topology. This is not device-tree discovery or a
portable hardware driver stack. Build tools are the existing RISC-V GCC and
binutils, QEMU, and Python 3.

## Authority and ownership

| World | User-accessible device mapping | IPC slots |
|---|---|---|
| Client (0) | None | 1/2 block request/reply; 3/4 console request/reply |
| Console (1) | UART page at `0x10000000`, RW/NX | 1 receive, 2 send |
| Block (2) | VirtIO page at `0x10008000`, RW/NX; private DMA page at `0x700000` | 1 receive, 2 send |

All three have separate copies of the linked user image, private heaps/stacks,
and the M6 RX text / RW non-executable data boundary. Sharing code in the build
does not share authority. No world receives the legacy `SYS_PUTC` capability.
The kernel retains emergency/test output through a supervisor-only UART alias
at `0x10010000`; SUM remains disabled. Device grants are static boot policy,
not a user-invokable mapping API. The console cannot access VirtIO, the block
service cannot access UART, and the client cannot access either or the DMA page.

Only the block service receives the DMA page's physical address, in startup
register a3. Virtqueue descriptors, available/used rings, request header, data,
and status fit in that 4 KiB page. A completed block is copied into a private
service buffer before IPC. The client receives copies in its own buffer.
There is no shared mutable client/service buffer and no pointer from a client
is interpreted as a DMA address.

**DMA trust is distinct from CPU isolation.** This target has no IOMMU: a
compromised block driver could program DMA outside its granted page. The driver
is trusted for DMA safety even though it runs in U mode. Deliberate driver-fault
tests occur before queue setup; they do not claim recovery from arbitrary
in-flight DMA. Normal stop and client-disconnect paths reset the device before
reusing DMA storage. Driver reset/timeout failures terminate acceptance rather
than attempt unsafe recovery. General device revocation, restart, and IOMMU
isolation remain future work.

## Service protocol

`services/protocol.h` defines a 64-bit header:

| Bits | Meaning |
|---|---|
| 0–7 | Operation: read=1, console=2, stop=3 |
| 8–31 | Request ID, echoed in completion |
| 32–63 | Request argument or completion status |

Each service has a dedicated request endpoint and reply endpoint. The current
protocol permits one outstanding request per service. IDs wrap at 24 bits and
are correlation values, not authority. Rights come from the endpoint capability.
Unknown operations and malformed arguments return BAD_REQUEST; out-of-capacity
reads return RANGE; unavailable devices or failed reads return IO. These are
service statuses, separate from kernel IPC_DENIED / IPC_PEER_GONE.

- Read takes a 32-bit sector number. A successful completion is followed by
  exactly 64 words (512 bytes), least-significant byte first. Error completions
  have no payload. The API reads only; it exposes no write or arbitrary command.
- Console takes a length from 1 to 512, followed by ceil(length/8) words. The
  service snapshots the complete bounded request before output. Unused bytes
  of the final word are ignored. Invalid lengths consume no payload. Exit
  halfway through a payload discards it without printing a partial string.
- Stop requires argument zero and closes the service after acknowledgement.

The client must inspect a completion before consuming block data or issuing the
next request. EOF is not silently substituted for a broken stream: a peer exit
or fault closes its endpoints and blocked senders/receivers get PEER_GONE.
A client that stays alive but stops supplying a payload can stall its dedicated
service; there are no per-request IPC deadlines or multiple-client fairness yet.

Requests and completions leave room for future asynchronous protocols, but M7
uses synchronous scalar rendezvous. A block reply currently requires 64 payload
messages. Console requests batch up to 512 bytes, packed eight per message.
This is a correctness baseline, not a zero-copy or throughput claim. Future
shared-buffer transport must specify ownership and validation anew; see
[performance investigation #28](https://github.com/farhanhubble/lispmacking/issues/28).

## VirtIO implementation

`services/virtio.c` implements modern MMIO version 2, negotiates VERSION_1,
checks FEATURES_OK, and configures split queue 0 with eight descriptor entries.
Each transfer uses a three-descriptor chain: header, 512-byte writable data,
and writable status. Descriptor addresses are derived exclusively from the
private DMA page. Memory/I/O fences publish descriptors and observe completion.
The driver polls with cooperative yields, validates used index/descriptor ID
and byte count, checks device status, and acknowledges pending interrupts.
It does not implement interrupt delivery, multiple outstanding requests,
indirect descriptors, packed rings, hotplug, or device writes.

Capacity reads use ConfigGeneration; setup and polling loops are bounded.
The acceptance driver also sends an unsupported request and checks the real
device's UNSUPP response before issuing reads. This probe is not exposed to
clients. Repeated reads exercise ring-slot reuse. Initialization failure reports
IO to the client; failures after device setup reset it before returning.
The device follows the [VirtIO 1.2 specification](https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.html),
sections on MMIO, split virtqueues and block devices; this is a deliberately
small subset, not a claim of full VirtIO conformance.

## Reproducible evidence

`tools/services/disk.py` builds four sectors: NUL-terminated source from
`tests/services/program.lisp`, a deterministic byte-pattern sector, and two
zero-filled sectors. Source is not embedded in the executable. The client checks
every byte of the pattern across ten reads, then reads and evaluates sector 0.

`tests/services/run.py` limits each QEMU process to 30 seconds and requires the
exact ordered 11-scenario transcript, successful QEMU exit, actual evaluation
output, and the surviving client's output after block-service failure:

1. Disk source evaluation and console output, including invalid rights,
   malformed requests, range checks, device error handling, and exact disk bytes.
2. Client UART access denied.
3. Client VirtIO access denied.
4. Client DMA page access denied.
5. Console fault wakes client.
6. Block fault wakes client; console remains usable.
7. Client exits halfway through console payload; services exit cleanly.
8. Client exits during block reply; the driver resets and exits cleanly.
9. Console cannot access the block device.
10. Block service cannot access UART.
11. Client cannot access the supervisor's UART alias.

Fault cases require cause 13 and the exact attempted address. Every survivor
also evaluates Lisp after the scenario. Two fresh-QEMU negative controls replace
the disk source (the expected-result assertion must fail) and omit the device
(the client must receive IO). Logs are saved in `build/services/acceptance.log`,
`changed-disk.log`, and `missing-device.log`.

The M6 kernel mechanisms are reused, with per-world device page tables and
boot-time DMA/device mappings. M7 policy and test progression are in
`services/demo.c`; services and VM integration are separate user objects.
This remains a bootable acceptance demonstrator with static worlds, no filesystem,
no interactive service shell, and no automatic driver restart.
