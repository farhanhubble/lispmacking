/* SPDX-License-Identifier: MPL-2.0 */
#ifndef MICROKERNEL_ABI_H
#define MICROKERNEL_ABI_H

#define USER_BASE 0x400000UL
#define WORLD_SIZE 0x200000UL
#define USER_TOP (USER_BASE + WORLD_SIZE)
#define STACK_SIZE 0x10000UL
#define STACK_GUARD (USER_TOP - STACK_SIZE - 4096)

/* a7 = operation, a0 = local capability handle, a1 = scalar argument.
 * Return a0 = status, a1 = received scalar. No user pointers cross this ABI. */
enum { SYS_EXIT, SYS_YIELD, SYS_SEND, SYS_RECEIVE, SYS_PUTC };
enum { IPC_OK = 0, IPC_DENIED = -1, IPC_PEER_GONE = -2, SYS_UNKNOWN = -3 };
enum {
    CASE_IPC,
    CASE_PREEMPT,
    CASE_KERNEL_READ,
    CASE_PEER_READ,
    CASE_UART_READ,
    CASE_TEXT_WRITE,
    CASE_DATA_EXEC,
    CASE_CSR,
    CASE_GUARD_READ,
    CASE_KERNEL_EXEC,
    CASE_PEER_EXIT,
    CASE_PEER_FAULT,
    CASE_NO_ENDPOINT,
    CASE_COUNT
};
#endif
