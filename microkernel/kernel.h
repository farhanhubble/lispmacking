/* SPDX-License-Identifier: MPL-2.0 */
#ifndef MICROKERNEL_KERNEL_H
#define MICROKERNEL_KERNEL_H
#include "microkernel/abi.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef SERVICES
#define WORLD_COUNT 3
#define CAP_COUNT 5
#define ENDPOINT_COUNT 4
#else
#define WORLD_COUNT 2
#define CAP_COUNT 4
#define ENDPOINT_COUNT 2
#endif

typedef struct {
    uint64_t x[32], pc, status;
} Frame;
_Static_assert(sizeof(Frame) == 272, "trap assembly frame layout");
_Static_assert(offsetof(Frame, pc) == 256, "trap assembly PC offset");
enum State { RUNNABLE, SENDING, RECEIVING, EXITED, FAULTED };
enum Right { RIGHT_NONE, RIGHT_SEND, RIGHT_RECEIVE, RIGHT_CONSOLE };
typedef struct {
    enum Right right;
    unsigned object;
} Capability;
typedef struct {
    Frame frame;
    enum State state;
    Capability caps[CAP_COUNT];
    uint64_t preemptions, last_preempt_pc, fault, fault_address, exit_code;
    uintptr_t peer_physical; /* Acceptance probe: knowing a PA conveys no access. */
} Thread;
extern Thread threads[WORLD_COUNT];

void kernel_text(const char *s);
void kernel_number(uint64_t value);
_Noreturn void kernel_fail(const char *s);
_Noreturn void kernel_finish(void);
void worlds_reset(unsigned scenario);
void grant(unsigned world, unsigned slot, enum Right right, unsigned object);
#ifdef SERVICES
void grant_device(unsigned world, uintptr_t address);
void grant_dma(unsigned world);
#endif
void retire(unsigned world, enum State state);
void demo_start(void);
void demo_progress(void);
#endif
