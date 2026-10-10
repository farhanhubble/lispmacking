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
    PAGE_SIZE = 4096,
    BOOTSTRAP_PAGE_COUNT = 8,
    MSTATUS_MIE = 1 << 3,
};

typedef struct {
    uintptr_t current;
    uintptr_t page_current;
    uintptr_t page_limit;
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
    if (allocator->current > allocator->page_limit ||
        aligned_size > allocator->page_limit - allocator->current) {
        return NULL;
    }
    result = allocator->current;
    allocator->current += aligned_size;
    return (void *)result;
}

static VmMachineResult page_allocate(void *context, uint64_t *address)
{
    Arena *allocator = context;
    uintptr_t page;
    size_t index;

    if (allocator->page_current < PAGE_SIZE) {
        return VM_MACHINE_OUT_OF_MEMORY;
    }
    page = allocator->page_current - PAGE_SIZE;
    if (page < allocator->page_limit) {
        return VM_MACHINE_OUT_OF_MEMORY;
    }
    allocator->page_current = page;
    for (index = 0; index < PAGE_SIZE; ++index) {
        ((volatile uint8_t *)page)[index] = 0;
    }
    *address = (uint64_t)page;
    return VM_MACHINE_OK;
}

static VmMachineResult validate_access(const Arena *allocator,
                                       uint64_t address, size_t width)
{
    if ((address & (uint64_t)(width - 1)) != 0) {
        return VM_MACHINE_MISALIGNED;
    }
    if (address < (uint64_t)allocator->page_current ||
        address > (uint64_t)allocator->end ||
        width > (uint64_t)allocator->end - address) {
        return VM_MACHINE_INVALID_ADDRESS;
    }
    return VM_MACHINE_OK;
}

static VmMachineResult memory_read(void *context, uint64_t address,
                                   size_t width, uint64_t *value)
{
    Arena *allocator = context;
    VmMachineResult result = validate_access(allocator, address, width);
    size_t index;
    uint64_t loaded = 0;

    if (result != VM_MACHINE_OK) {
        return result;
    }
    for (index = 0; index < width; ++index) {
        loaded |= (uint64_t)((volatile uint8_t *)(uintptr_t)address)[index]
                  << (index * 8);
    }
    *value = loaded;
    return VM_MACHINE_OK;
}

static VmMachineResult memory_write(void *context, uint64_t address,
                                    size_t width, uint64_t value)
{
    Arena *allocator = context;
    VmMachineResult result = validate_access(allocator, address, width);
    size_t index;

    if (result != VM_MACHINE_OK) {
        return result;
    }
    for (index = 0; index < width; ++index) {
        ((volatile uint8_t *)(uintptr_t)address)[index] =
            (uint8_t)(value >> (index * 8));
    }
    return VM_MACHINE_OK;
}

static VmMachineResult csr_read(void *context, VmCsr csr, uint64_t *value)
{
    uintptr_t result;

    (void)context;
    switch (csr) {
    case VM_CSR_MSTATUS:
        __asm__ volatile("csrr %0, mstatus" : "=r"(result));
        break;
    case VM_CSR_MIE:
        __asm__ volatile("csrr %0, mie" : "=r"(result));
        break;
    case VM_CSR_MTVEC:
        __asm__ volatile("csrr %0, mtvec" : "=r"(result));
        break;
    case VM_CSR_MSCRATCH:
        __asm__ volatile("csrr %0, mscratch" : "=r"(result));
        break;
    default:
        return VM_MACHINE_INVALID_OPERATION;
    }
    *value = (uint64_t)result;
    return VM_MACHINE_OK;
}

static VmMachineResult csr_write(void *context, VmCsr csr, uint64_t value)
{
    uintptr_t native_value = (uintptr_t)value;

    (void)context;
    switch (csr) {
    case VM_CSR_MIE:
        __asm__ volatile("csrw mie, %0" : : "r"(native_value) : "memory");
        break;
    case VM_CSR_MTVEC:
        __asm__ volatile("csrw mtvec, %0" : : "r"(native_value) : "memory");
        break;
    case VM_CSR_MSCRATCH:
        __asm__ volatile("csrw mscratch, %0" : : "r"(native_value) : "memory");
        break;
    case VM_CSR_MSTATUS:
    default:
        return VM_MACHINE_INVALID_OPERATION;
    }
    return VM_MACHINE_OK;
}

static VmMachineResult interrupts_disable(void *context,
                                           uint64_t *previously_enabled)
{
    uintptr_t previous;

    (void)context;
    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(previous)
                     : "r"((uintptr_t)MSTATUS_MIE)
                     : "memory");
    *previously_enabled = (previous & MSTATUS_MIE) != 0 ? 1u : 0u;
    return VM_MACHINE_OK;
}

static VmMachineResult interrupts_restore(void *context,
                                           uint64_t previously_enabled)
{
    (void)context;
    if (previously_enabled != 0) {
        __asm__ volatile("csrs mstatus, %0"
                         :
                         : "r"((uintptr_t)MSTATUS_MIE)
                         : "memory");
    } else {
        __asm__ volatile("csrc mstatus, %0"
                         :
                         : "r"((uintptr_t)MSTATUS_MIE)
                         : "memory");
    }
    return VM_MACHINE_OK;
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
    arena.page_current = (uintptr_t)__heap_end;
    arena.page_limit = (uintptr_t)__heap_end -
                       BOOTSTRAP_PAGE_COUNT * PAGE_SIZE;
    arena.end = (uintptr_t)__heap_end;
    platform = (VmPlatform){0};
    platform.context = &arena;
    platform.allocate = arena_allocate;
    platform.deallocate = arena_deallocate;
    platform.page_allocate = page_allocate;
    platform.memory_read = memory_read;
    platform.memory_write = memory_write;
    platform.csr_read = csr_read;
    platform.csr_write = csr_write;
    platform.interrupts_disable = interrupts_disable;
    platform.interrupts_restore = interrupts_restore;
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
