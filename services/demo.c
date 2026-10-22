/* SPDX-License-Identifier: MPL-2.0 */
#include "microkernel/kernel.h"
#include "services/protocol.h"
static unsigned scenario;
static const char *const names[] = {"disk Lisp through console service",
                                    "client UART denied",
                                    "client VirtIO denied",
                                    "client DMA denied",
                                    "console fault wakes client",
                                    "block fault wakes client",
                                    "client exit releases services",
                                    "client exit during block reply",
                                    "console cannot access block device",
                                    "block cannot access UART",
                                    "client kernel UART alias denied"};
void demo_start(void)
{
    worlds_reset(scenario);
    /* Client 0, console 1, block 2. No world receives SYS_PUTC authority. */
    grant(0, 1, RIGHT_SEND, 0);
    grant(2, 1, RIGHT_RECEIVE, 0);
    grant(2, 2, RIGHT_SEND, 1);
    grant(0, 2, RIGHT_RECEIVE, 1);
    grant(0, 3, RIGHT_SEND, 2);
    grant(1, 1, RIGHT_RECEIVE, 2);
    grant(1, 2, RIGHT_SEND, 3);
    grant(0, 4, RIGHT_RECEIVE, 3);
    grant_device(1, UART_BASE);
    grant_device(2, BLOCK_BASE);
    grant_dma(2);
    kernel_text("CASE: ");
    kernel_text(names[scenario]);
    kernel_text("\n");
}
void demo_progress(void)
{
    int bad = scenario >= CLIENT_UART && scenario <= CLIENT_DMA        ? 0
              : scenario == CONSOLE_FAULT || scenario == CONSOLE_BLOCK ? 1
              : scenario == BLOCK_FAULT || scenario == BLOCK_UART      ? 2
              : scenario == CLIENT_KERNEL_UART                         ? 0
                                                                       : -1;
    uintptr_t address = scenario == CLIENT_UART          ? UART_BASE
                        : scenario == CLIENT_BLOCK       ? BLOCK_BASE
                        : scenario == CLIENT_DMA         ? DMA_VA
                        : scenario == CONSOLE_BLOCK      ? BLOCK_BASE
                        : scenario == BLOCK_UART         ? UART_BASE
                        : scenario == CLIENT_KERNEL_UART ? 0x10010000UL
                                                         : 0x80000000UL;
    bool done = true;
    for (unsigned i = 0; i < WORLD_COUNT; ++i) {
        Thread *t = &threads[i];
        if (t->state == EXITED && t->exit_code) {
            kernel_text("world ");
            kernel_number(i);
            kernel_text(" exit=");
            kernel_number(t->exit_code);
            kernel_text("\n");
            kernel_fail("service assertion");
        }
        if (t->state == FAULTED &&
            ((int)i != bad || t->fault != 13 || t->fault_address != address)) {
            kernel_text("world ");
            kernel_number(i);
            kernel_text(" cause=");
            kernel_number(t->fault);
            kernel_text(" address=");
            kernel_number(t->fault_address);
            kernel_text("\n");
            kernel_fail("unexpected service fault");
        }
        if (t->state < EXITED)
            done = false;
    }
    if (!done)
        return;
    if (bad >= 0 && threads[bad].state != FAULTED)
        kernel_fail("expected service fault missing");
    kernel_text("PASS: ");
    kernel_text(names[scenario]);
    kernel_text("\n");
    if (++scenario == SERVICE_CASES)
        kernel_finish();
    demo_start();
}
