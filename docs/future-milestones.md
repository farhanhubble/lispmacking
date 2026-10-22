# Detailed milestones M8–M19

Status: planned, not implemented. These definitions extend the completed M0–M7
roadmap. Numbers identify deliverables; dependencies determine execution order.
No dates or performance targets are promised before measurement.

The design follows the Lispmacking seed: C remains bootstrap scaffolding;
compiled Lisp increasingly implements the system. Language semantics are shared
across hosted, RV64 and RV32 targets, while protection and device guarantees are
explicitly target-specific. ESP32 support is RISC-V only, beginning with C3.

Every milestone requires reproducible commands, versioned fixtures, documented
limits, negative tests and relevant existing regressions. Emulator success does
not establish silicon timing or protection correctness. A milestone with a
physical-board acceptance requirement remains open until that evidence exists.

## M8 — Interactive Lisp workspace

**Outcome:** Use the isolated system interactively without restarting it to
reclaim every temporary allocation.

**Dependencies:** M7. Develop the memory/platform interfaces alongside M9;
completion of the ESP32 port is not a prerequisite.

**Scope:** Service-backed console input, line editing at a bounded scale,
reader/evaluator error recovery, named program loading from a reproducible
read-only image, and memory reclamation that preserves live definitions and
closures. Define roots and allocation ownership before introducing a collector.
Reclamation must not run in interrupt or context-switch paths. Expose useful
heap/stack diagnostics and document limits on recursion and input size.

**Acceptance:**

- [ ] Boot to a user-world REPL and evaluate injected serial input.
- [ ] Load a named program through the block service and invoke its definitions.
- [ ] Run a bounded long-session test that allocates beyond the total heap size
  cumulatively while retaining live bindings and reclaiming unreachable values.
- [ ] Recover from malformed input, missing programs and allocation exhaustion;
  verify a subsequent valid expression succeeds and another world survives.
- [ ] Document reclamation pauses and resource bounds; preserve M4–M7 tests.

**Validation:** Hosted semantic/reclamation tests and RV64 QEMU serial scenarios.
**Excluded:** Persistent object images, a full editor, writable filesystem,
unbounded recursion and a hard real-time garbage collector.

## M9 — ESP32-C3 runtime

**Outcome:** Run the portable Lisp evaluator on a resource-constrained RV32
microcontroller using ESP-IDF as initial scaffolding.

**Dependencies:** M7 portable VM baseline; coordinate reclamation with M8.
Bring-up can begin before M8 completes.

**Scope:** Separate pointer width from 64-bit language integers; audit alignment,
size conversions, stack use, allocation and unsupported operations. Add an
ESP-IDF adapter with configurable heap/stack budgets, serial REPL and a bounded
GPIO interface. Pin ESP-IDF, toolchain and esp-emulator versions. Provide reusable
build, run, test and flash commands; document the selected board and GPIO wiring.

**Acceptance:**

- [ ] Shared language conformance fixtures pass on hosted, RV64 and C3 targets,
  including 64-bit arithmetic boundaries on a 32-bit CPU.
- [ ] Automated esp-emulator tests inject serial expressions and verify results,
  input errors, unsupported operations and recoverable allocation exhaustion.
- [ ] Record flash, static RAM, heap and measured stack use against explicit
  budgets for the selected board; sustain a repeated-allocation workload.
- [ ] Run the serial suite and a safe GPIO demonstration on a physical C3;
  record board/chip revision, wiring, firmware hash and transcript.

**Validation:** Hosted/RV64 regressions, pinned emulator and physical C3.
**Excluded:** Xtensa ESP32, our own C3 kernel, Wi-Fi/BLE integration, SIMD and a
claim of hardware-isolated Lisp worlds under the initial IDF adapter.

## M10 — Native systems Lisp

**Outcome:** Move native compilation into protected worlds and replace a
meaningful C service implementation with compiled Lisp.

**Dependencies:** M8 for the interactive RV64 workflow; M9 for the RV32 acceptance
leg. M11 is not required: use an explicitly restricted systems-language subset.

**Scope:** Define target ABIs, value representation, overflow behavior, calls,
branches, bounded memory access and service bindings. Add controlled code loading,
relocations and instruction-cache synchronisation; writable code construction
must not leave a writable executable alias. Add an RV32 backend, including an
explicit strategy for 64-bit language values. Define non-allocating systems
functions and restrictions; retain the interpreter as a semantic reference.

**Acceptance:**

- [ ] Compile, install and call a function in an RV64 U-mode world and compare
  arithmetic, branch, call and error behavior with the reference evaluator.
- [ ] Deny writes to installed executable code and execution of writable data;
  malformed code-loading requests fail without extending caller authority.
- [ ] Replace a named C service implementation, such as console protocol/output
  policy, with compiled Lisp while retaining service acceptance tests.
- [ ] Compile and run supported functions on C3, including defined integer
  boundary behavior; record physical-board and emulator results.
- [ ] Document what remains C/assembly and the supported source subset.

**Validation:** Hosted differential tests, RV64 protection tests, emulator/C3.
**Excluded:** Complete self-hosting, arbitrary Common Lisp compilation, kernel
rewrites, SIMD, and assuming generated native code is safe merely because it is Lisp.

## M11 — Language and compiler extensibility

**Outcome:** Extend language syntax and machine-code generation through explicit
interfaces rather than accumulating special cases in the C evaluator.

**Dependencies:** M10; modules and macro design can be explored earlier.

**Scope:** Define macro expansion and name-capture rules, module/namespace boundaries,
compiler intermediate representation and lowering hooks. Introduce typed buffers
with checked lengths, alignment, ownership and lifetime; define integer and
floating-point representations, conversions, rounding and fallback behavior.
Target descriptions advertise actual ISA capabilities and ABI constraints.

**Acceptance:**

- [ ] Implement a macro-based DSL and module imports with deterministic expansion;
  test name collisions, missing imports and malformed expansions.
- [ ] Add a native lowering through the documented extension interface without
  modifying evaluator dispatch; compare it with a portable implementation.
- [ ] Reject out-of-bounds, misaligned and invalid-lifetime buffer operations.
- [ ] Test the declared numeric subset, including conversions and exceptional
  floating-point cases, with explicit target differences and software fallbacks.
- [ ] Reproducibly rebuild the extension and run hosted/RV64/RV32 conformance.

**Validation:** Compiler fixtures and differential tests across supported targets.
**Excluded:** Automatic vectorisation, arbitrary compiler-plugin sandboxing,
complete numeric towers and native accelerator drivers. Compiler extensions run
with documented build-world authority, not implicit kernel privilege.

## M12 — Persistent language workspace

**Outcome:** Restore useful definitions, objects and namespaces across boots with
explicit consistency and authority rules.

**Dependencies:** M8 and M11 for the versioned object/module model; M7 storage.
Storage-write and recovery prototypes can start earlier.

**Scope:** Add bounded block writes and a small versioned storage format. Define
commit boundaries, checksums, recovery, object identity, reference graphs and
schema migration. Persist code as an explicitly versioned representation;
reject or rebuild incompatible native artifacts. External capabilities must be
re-authorised or rebound through policy, never restored as raw privileged handles.

**Acceptance:**

- [ ] Save a workspace containing definitions, namespaces and shared/cyclic
  object references; reboot and verify identity relationships and behavior.
- [ ] Inject interruption at documented write/commit boundaries and recover a
  complete committed version without silently exposing partial state.
- [ ] Handle full media, corrupt/truncated records and incompatible versions.
- [ ] Demonstrate that a saved device/endpoint reference cannot regain authority
  without an explicit restore policy.
- [ ] Publish the storage format, recovery algorithm and fixture-generation tools.

**Validation:** Hosted format/fault-injection tests and fresh-QEMU reboot tests.
**Excluded:** POSIX filesystem compatibility, distributed transactions and C3
power-loss guarantees without separate flash-specific hardware validation.

## M13 — Managed worlds and service lifecycle

**Outcome:** Create and retire bounded worlds while managing authority and
resources throughout their lifetime.

**Dependencies:** M10 code loading and M8 reclamation. Independent of persistence.

**Scope:** World/thread/endpoint creation, explicit initial grants, memory/CPU
budgets, capability derivation and revocation semantics, and failure cleanup.
Prevent stale handles from naming reused objects. Define service reconnection
and recovery. Treat DMA device shutdown separately from ordinary process teardown.

**Acceptance:**

- [ ] Launch programs with different rights; deny ungranted operations and
  enforce resource limits without starving an unrelated world.
- [ ] Repeatedly terminate worlds and verify pages, endpoints and kernel objects
  are reclaimed; stale handles and revoked descendants remain unusable.
- [ ] Wake blocked peers on failure and demonstrate a controlled restart of a
  named service with explicit client reconnection semantics.
- [ ] Define and test a DMA quiescence boundary before any device buffer reuse.
  If device revocation cannot be guaranteed, refuse restart and document it.
- [ ] Document the trusted components and what a compromised driver can still do.

**Validation:** RV64 QEMU lifecycle/fault tests plus hosted object-table tests.
**Excluded:** Transparent recovery of arbitrary in-flight operations and a claim
of malicious-DMA containment without appropriate hardware protection. IOMMU-based
isolation requires an explicit platform-specific follow-up before such a claim.

## M14 — Native ESP32-C3 kernel

**Outcome:** Run Lispmacking's own kernel on C3, distinct from the M9 IDF runtime.

**Dependencies:** M9 board/toolchain evidence and M10 RV32 code/ABI support.
M13 informs lifecycle APIs but is not required for a static-world first kernel.

**Scope:** Chip-specific boot, memory layout, traps, timer, interrupt routing,
watchdog handling, scheduling and region-based protection. Audit C3 privilege,
PMP and peripheral permission behavior against its technical reference and
silicon. Define RV32 syscall payload handling and service authority without
assuming Sv39 virtual address spaces. Keep the IDF runtime as a separate target.

**Acceptance:**

- [ ] Boot our kernel and schedule at least two bounded worlds with a documented
  physical memory/protection layout and saved architectural state.
- [ ] Preempt a non-yielding world; demonstrate explicitly granted IPC.
- [ ] On physical C3, deny peer/kernel memory and ungranted peripheral accesses,
  verify expected faults and continued execution of a healthy peer.
- [ ] Test trap recovery, stack bounds, allocation/resource exhaustion and
  board reset behavior; report emulator/silicon differences.
- [ ] Document exact isolation and DMA limits, boot dependencies and flash layout.

**Validation:** Pinned esp-emulator plus physical C3; silicon evidence is mandatory.
**Excluded:** Xtensa, pretending PMP is paged virtual memory, Wi-Fi/BLE drivers,
multicore, and dynamic worlds unless separately implemented.

## M15 — Vector/SIMD execution

**Outcome:** Add the first optional vector backend while preserving scalar
semantics and isolation between vector-using worlds.

**Dependencies:** M10 native U-mode execution and M11 numeric/compiler interfaces.
Does not require SMP, persistence or ESP32 vector support.

**Scope:** A bounded RISC-V V backend with explicit element types, masks, tail
handling, alignment and numeric rules. Define vector ABI and kernel state
management across preemption, faults and world retirement, including control
state and initial-state confidentiality. Retain scalar fallbacks and feature
checks; do not assume fixed vector width or require C3 to support V.

**Acceptance:**

- [ ] Compile vector arithmetic and a reduction with scalar reference results;
  test zero lengths, tails, masks, alignment and declared floating-point tolerance.
- [ ] Run under at least two supported vector-length configurations in QEMU.
- [ ] Repeatedly preempt two vector-using worlds with distinct register patterns;
  detect corruption or cross-world state disclosure, including newly created state.
- [ ] Unsupported targets use a documented fallback or explicit error.
- [ ] Record instruction evidence and reproducible correctness tests; hardware
  speedup claims require real target measurements.

**Validation:** Hosted reference tests and vector-enabled RV64 QEMU; physical
vector hardware for any performance claims.
**Excluded:** General auto-vectorisation, GPU execution, mandatory floating-point
support on every MCU, and broad cryptographic side-channel guarantees.

## M16 — Asynchronous services

**Outcome:** Overlap independent computation and service work on a single core
before introducing cross-core execution.

**Dependencies:** M13 endpoint/lifecycle semantics and M11 buffer ownership model.
Protocol prototypes can use static worlds earlier.

**Scope:** Multiple outstanding requests, bounded queues, completion correlation,
batching, notifications and backpressure. Define copied versus shared-buffer
ownership, cancellation, timeout races and peer failure. Add interrupt-driven
completion for a selected device path; no blocking or allocation in low-level
interrupt handling. Integrate a measured baseline from issue #28.

**Acceptance:**

- [ ] Submit multiple requests, perform independent computation and consume
  completions with correct IDs and documented ordering.
- [ ] Test full queues, slow consumers, cancellation/completion races, timeouts,
  peer death and invalid buffer access without leaks or use-after-free.
- [ ] Demonstrate real device completion through the defined notification path.
- [ ] Compare equivalent synchronous and asynchronous workloads, recording latency,
  throughput and CPU cost separately; make no unconditional speedup promise.

**Validation:** Hosted protocol stress tests and single-hart RV64 QEMU; real
hardware for physical performance results.
**Excluded:** SMP, unbounded queues, transparent retry of non-idempotent requests,
and general zero-copy claims without ownership and lifetime evidence.

## M17 — Multicore execution and measurement

**Outcome:** Safely execute worlds on multiple cores and measure when cross-core
communication helps or hurts.

**Dependencies:** M13 lifecycle and M16 asynchronous protocols. If vector execution
is enabled, M15 state rules must remain correct across permitted migrations.

**Scope:** Secondary-hart boot, per-core scheduler state, kernel synchronisation,
interprocessor notifications, memory ordering, TLB invalidation/shootdown and
safe reclamation. Define affinity, migration and supported homogeneous CPU
features. Protect shared queues from cache-line contention where measurement
justifies it. Extend the investigation in issue #28.

**Acceptance:**

- [ ] Run isolated workloads concurrently on two or more RV64 harts.
- [ ] Stress cross-core IPC, revocation, teardown, migration and page reuse;
  stale translations or handles cannot restore access.
- [ ] Test CPU-bound and I/O-bound workloads, fairness and bounded queues.
- [ ] Compare same-core handoff, cross-core requests and pinned service workers
  under equivalent work; report latency distributions, throughput and CPU cost.
- [ ] Publish hardware/compiler/configuration details and raw measurements;
  label QEMU results as functional evidence, not silicon performance.

**Validation:** Multihart QEMU correctness plus a selected physical multicore
RISC-V platform for the performance acceptance report.
**Excluded:** C3 multicore support, NUMA optimisation, heterogeneous scheduling
and assuming additional cores always reduce request latency.

## M18 — First accelerator package

**Outcome:** Express a computation in Lisp and execute it through a host-side GPU
service with explicit authority, buffers and completion.

**Dependencies:** M11 compiler extensions and M16 asynchronous service contracts.
M17 is optional; begin with a declared host/guest transport and trust boundary.

**Scope:** Device-language subset, typed buffers, lowering, launch stubs, events,
data movement, resource quotas and scalar fallback. Select one supported host
GPU/backend and pin its toolchain. The host service owns the vendor driver;
the Lisp client receives narrow remote operations, not arbitrary host pointers.

**Acceptance:**

- [ ] Compile and run a Lisp-expressed vector or matrix workload on a real GPU;
  compare with a CPU reference using documented numerical rules.
- [ ] Validate bounds, buffer lifetime, access rights and completion IDs.
- [ ] Recover predictably from service disconnection, device error and timeout;
  reclaim resources and report partial/unknown execution explicitly.
- [ ] Run a fallback when no accelerator is present and verify matching semantics.
- [ ] Report transfer, launch and compute costs separately, with reproducible
  host/device/toolchain details and explicit trusted components.

**Validation:** Protocol tests without a GPU and mandatory real-GPU demonstration.
**Excluded:** Native NVIDIA/AMD kernel drivers, transparent GPU isolation, and
simultaneous tensor-core, FPGA, DSP or NPU backends. Each later backend needs its
own capability, compiler, lifecycle and acceptance definition.

## M19 — Connected and recursive worlds

**Outcome:** Connect an ESP32 service node and demonstrate one intentionally
nested, bounded language environment with deterministic replay.

**Dependencies:** M9 for a trusted IDF-based C3 node, or M14 when native-kernel
isolation is claimed; M13 lifecycle and M16 protocol semantics. M18 is optional.

**Scope:** Two explicit deliverables: (1) a serial or network service protocol for
a C3 sensor/actuator node, with authentication/authorisation appropriate to the
selected transport, request IDs, resource limits and disconnect recovery;
(2) a nested world with bounded memory, virtual time, restricted external effects
and recorded inputs. Distinguish logical interpreter confinement from hardware
isolation; do not imply arbitrary native subworld code is safely sandboxed.

**Acceptance:**

- [ ] Send an authorised bounded operation to a physical C3 and verify a real
  sensor/actuator or documented GPIO fixture response.
- [ ] Test malformed/replayed/unauthorised requests, disconnect/reconnect and
  uncertain completion; avoid duplicate non-idempotent actions.
- [ ] Run a nested world with restricted capabilities and resource limits;
  preserve its parent after a controlled child failure.
- [ ] Record inputs and virtual-time events, replay the bounded workload and
  verify deterministic observable results under a documented execution model.
- [ ] Publish the transport protocol, threat model and replay limitations.

**Validation:** Hosted replay tests, RV64 integration, esp-emulator protocol tests
and physical C3 evidence. No requirement to implement C3 Wi-Fi initially.
**Excluded:** Transparent distributed shared memory, whole-machine deterministic
replay, untrusted-native-code sandboxing without a protection mechanism and
nested interpretation as the default execution model.
