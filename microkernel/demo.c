/* SPDX-License-Identifier: MPL-2.0 */
/* Boot policy and acceptance orchestration are separate from kernel mechanisms. */
#include "build/microkernel/image_layout.h"
#include "microkernel/kernel.h"
static unsigned scenario;
static const char *const names[CASE_COUNT] = {
    "isolated Lisp IPC",           "timer preemption",
    "kernel read denied",          "peer physical read denied",
    "UART access denied",          "text write denied",
    "data execute denied",         "privileged CSR denied",
    "stack guard fault",           "kernel execute denied",
    "peer exit wakes receiver",    "peer fault wakes sender",
    "no endpoint grant denies IPC"};
static const unsigned faults[CASE_COUNT] = {0, 0,  13, 13, 13, 15, 12,
                                            2, 13, 12, 0,  13, 0};

void demo_start(void)
{
    worlds_reset(scenario);
    grant(0, 0, RIGHT_CONSOLE, 0);
    grant(1, 0, RIGHT_CONSOLE, 0);
    if (scenario != CASE_NO_ENDPOINT) {
        grant(0, 1, RIGHT_SEND, 0);
        grant(1, 1, RIGHT_RECEIVE, 0);
        grant(0, 2, RIGHT_RECEIVE, 1);
        grant(1, 2, RIGHT_SEND, 1);
    }
    kernel_text("CASE: ");
    kernel_text(names[scenario]);
    kernel_text("\n");
}

static uintptr_t expected_address(void)
{
    switch (scenario) {
    case CASE_KERNEL_READ:
    case CASE_KERNEL_EXEC:
    case CASE_PEER_FAULT:
        return 0x80000000UL;
    case CASE_PEER_READ:
        return threads[0].peer_physical;
    case CASE_UART_READ:
        return 0x10000000UL;
    case CASE_TEXT_WRITE:
        return USER_BASE;
    case CASE_DATA_EXEC:
        return USER_DATA_PROBE;
    case CASE_GUARD_READ:
        return STACK_GUARD;
    default:
        return 0;
    }
}

void demo_progress(void)
{
    unsigned fault_world = scenario == CASE_PEER_FAULT ? 1 : 0;
    for (unsigned i = 0; i < WORLD_COUNT; ++i) {
        if (threads[i].state == EXITED && threads[i].exit_code) {
            kernel_text("world ");
            kernel_number(i);
            kernel_text(" exit=");
            kernel_number(threads[i].exit_code);
            kernel_text("\n");
            kernel_fail("user assertion");
        }
        if (threads[i].state == FAULTED && (i != fault_world || !faults[scenario] ||
                                            threads[i].fault != faults[scenario])) {
            kernel_text("world ");
            kernel_number(i);
            kernel_text(" cause=");
            kernel_number(threads[i].fault);
            kernel_text(" address=");
            kernel_number(threads[i].fault_address);
            kernel_text(" pc=");
            kernel_number(threads[i].frame.pc);
            kernel_text("\n");
            kernel_fail("unexpected user fault");
        }
    }
    if (scenario == CASE_PREEMPT && threads[1].state == EXITED) {
        if (!threads[0].preemptions || threads[0].last_preempt_pc < USER_SPIN_BEGIN ||
            threads[0].last_preempt_pc >= USER_SPIN_END)
            return;
        retire(0, EXITED);
    }
    if (threads[0].state < EXITED || threads[1].state < EXITED)
        return;
    if (faults[scenario] && threads[fault_world].state != FAULTED)
        kernel_fail("forbidden access did not fault");
    if (faults[scenario] && expected_address() &&
        threads[fault_world].fault_address != expected_address())
        kernel_fail("fault was not at the deliberately forbidden address");
    kernel_text("PASS: ");
    kernel_text(names[scenario]);
    if (faults[scenario]) {
        kernel_text(" cause=");
        kernel_number(threads[fault_world].fault);
    }
    kernel_text("\n");
    if (++scenario == CASE_COUNT)
        kernel_finish();
    demo_start();
    /* The scheduler must start world 0 even after a non-rotating syscall. */
}
