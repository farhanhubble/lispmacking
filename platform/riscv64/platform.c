/* SPDX-License-Identifier: MPL-2.0 */

#include "platform/riscv64/platform.h"

#include <stdint.h>

enum {
    UART0_BASE = 0x10000000,
    UART_RBR = 0,
    UART_THR = 0,
    UART_LSR = 5,
    UART_LSR_DATA_READY = 1 << 0,
    UART_LSR_THR_EMPTY = 1 << 5,
    QEMU_TEST_BASE = 0x100000,
    QEMU_TEST_PASS = 0x5555,
    ALLOCATION_ALIGNMENT = 16,
};

typedef struct {
    uintptr_t current;
    uintptr_t end;
} Arena;

extern unsigned char __heap_start[];
extern unsigned char __heap_end[];

static volatile uint8_t *const uart0 = (volatile uint8_t *)UART0_BASE;
static Arena arena;

static void uart_putc(char character)
{
    while ((uart0[UART_LSR] & UART_LSR_THR_EMPTY) == 0) {
    }
    uart0[UART_THR] = (uint8_t)character;
}

static char uart_getc(void)
{
    while ((uart0[UART_LSR] & UART_LSR_DATA_READY) == 0) {
    }
    return (char)uart0[UART_RBR];
}

static void *arena_allocate(void *context, size_t size)
{
    Arena *allocator = context;
    uintptr_t aligned_size;
    uintptr_t result;

    if (size > UINTPTR_MAX - (ALLOCATION_ALIGNMENT - 1)) {
        return NULL;
    }
    aligned_size = ((uintptr_t)size + ALLOCATION_ALIGNMENT - 1) &
                   ~(uintptr_t)(ALLOCATION_ALIGNMENT - 1);
    if (aligned_size > allocator->end - allocator->current) {
        return NULL;
    }
    result = allocator->current;
    allocator->current += aligned_size;
    return (void *)result;
}

static void arena_deallocate(void *context, void *pointer)
{
    (void)context;
    (void)pointer;
}

VmPlatform riscv64_platform(void)
{
    VmPlatform platform;

    arena.current = (uintptr_t)__heap_start;
    arena.end = (uintptr_t)__heap_end;
    platform.context = &arena;
    platform.allocate = arena_allocate;
    platform.deallocate = arena_deallocate;
    return platform;
}

void riscv64_console_write(void *context, const char *bytes, size_t length)
{
    size_t index;

    (void)context;
    for (index = 0; index < length; ++index) {
        uart_putc(bytes[index]);
    }
}

Riscv64LineResult riscv64_console_read_line(char *buffer, size_t capacity)
{
    size_t length = 0;
    bool too_long = false;

    if (capacity == 0) {
        return RISCV64_LINE_TOO_LONG;
    }
    for (;;) {
        char character = uart_getc();

        if (character == '\004') {
            riscv64_console_write(NULL, "\r\n", 2);
            return RISCV64_LINE_END;
        }
        if (character == '\r' || character == '\n') {
            buffer[length] = '\0';
            riscv64_console_write(NULL, "\r\n", 2);
            return too_long ? RISCV64_LINE_TOO_LONG : RISCV64_LINE_READY;
        }
        if (character == '\b' || character == 0x7f) {
            if (length != 0) {
                --length;
                riscv64_console_write(NULL, "\b \b", 3);
            }
            continue;
        }
        if ((unsigned char)character < 0x20) {
            continue;
        }
        if (length + 1 < capacity) {
            buffer[length++] = character;
            uart_putc(character);
        } else {
            too_long = true;
        }
    }
}

_Noreturn void riscv64_qemu_exit(void)
{
    *(volatile uint32_t *)QEMU_TEST_BASE = QEMU_TEST_PASS;
    for (;;) {
        __asm__ volatile("wfi");
    }
}
