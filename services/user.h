/* SPDX-License-Identifier: MPL-2.0 */
#ifndef SERVICES_USER_H
#define SERVICES_USER_H
#include "microkernel/abi.h"
#include "services/protocol.h"
#include <stdbool.h>
#include <stddef.h>
typedef struct {
    int64_t status;
    uint64_t value;
} Reply;
static inline Reply call(uint64_t op, uint64_t cap, uint64_t value)
{
    register uint64_t a0 __asm__("a0") = cap;
    register uint64_t a1 __asm__("a1") = value;
    register uint64_t a7 __asm__("a7") = op;
    __asm__ volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a7) : "memory");
    return (Reply){(int64_t)a0, a1};
}
static inline _Noreturn void finish(uint64_t status)
{
    call(SYS_EXIT, status, 0);
    for (;;) {
    }
}
static inline void require(bool condition, unsigned code)
{
    if (!condition)
        finish(code);
}
void console_service(unsigned scenario);
void block_service(unsigned scenario, uintptr_t physical);
bool block_init(uintptr_t physical);
unsigned block_read(uint64_t sector, uint64_t output[64]);
bool block_error_probe(void);
void block_close(void);
#endif
