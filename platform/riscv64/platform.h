#ifndef LISP_MACHINE_RISCV64_PLATFORM_H
#define LISP_MACHINE_RISCV64_PLATFORM_H

#include "vm/vm.h"

typedef enum {
    RISCV64_LINE_READY,
    RISCV64_LINE_END,
    RISCV64_LINE_TOO_LONG,
} Riscv64LineResult;

VmPlatform riscv64_platform(void);
void riscv64_console_write(void *context, const char *bytes, size_t length);
Riscv64LineResult riscv64_console_read_line(char *buffer, size_t capacity);
_Noreturn void riscv64_qemu_exit(void);

#endif
