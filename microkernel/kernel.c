/* SPDX-License-Identifier: MPL-2.0 */
#include "microkernel/kernel.h"
#ifndef IMAGE_LAYOUT_HEADER
#define IMAGE_LAYOUT_HEADER "build/microkernel/image_layout.h"
#endif
#include IMAGE_LAYOUT_HEADER
#ifdef SERVICES
#include "services/protocol.h"
#endif

enum {
    PTE_V = 1,
    PTE_R = 2,
    PTE_W = 4,
    PTE_X = 8,
    PTE_U = 16,
    PTE_A = 64,
    PTE_D = 128
};
#define PAGE 4096UL
#define LEAF (PTE_V | PTE_A | PTE_D)

Thread threads[WORLD_COUNT];
static unsigned current;
static bool fresh;
/* Physical backing never appears in a U-accessible mapping at its PA. */
static _Alignas(PAGE) unsigned char memory[WORLD_COUNT][WORLD_SIZE];
static _Alignas(PAGE) uint64_t roots[WORLD_COUNT][512];
static _Alignas(PAGE) uint64_t lower[WORLD_COUNT][512];
static _Alignas(PAGE) uint64_t leaves[WORLD_COUNT][512];
static _Alignas(PAGE) uint64_t devices[WORLD_COUNT][512];
#ifdef SERVICES
static _Alignas(PAGE) unsigned char dma_page[PAGE];
static _Alignas(PAGE) uint64_t dma_leaves[WORLD_COUNT][512];

#endif
static _Alignas(PAGE) uint64_t test_device[512];

typedef struct {
    int sender, receiver, waiting_sender, waiting_receiver;
    uint64_t value;
    bool closed;
} Endpoint;
static Endpoint endpoints[ENDPOINT_COUNT];
extern unsigned char user_image_start[], user_image_end[];
extern void supervisor_trap(void);
extern _Noreturn void restore_user(Frame *frame);

static void zero(void *address, size_t size)
{
    unsigned char *p = address;
    for (size_t i = 0; i < size; ++i)
        p[i] = 0;
}

static uint64_t pte(uintptr_t address, uint64_t flags)
{
    return ((address >> 12) << 10) | flags;
}

static void putc(char c)
{
    uintptr_t address = 0x10000000UL;
#ifdef SERVICES
    uintptr_t satp;
    __asm__ volatile("csrr %0, satp" : "=r"(satp));
    if (satp)
        address = 0x10010000UL; /* S-only alias; SUM stays disabled. */
#endif
    volatile unsigned char *uart = (void *)address;
    while (!(uart[5] & 32)) {
    }
    uart[0] = (unsigned char)c;
}

void kernel_text(const char *s)
{
    while (*s)
        putc(*s++);
}

void kernel_number(uint64_t value)
{
    char digits[21];
    unsigned n = 0;
    do {
        digits[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (n)
        putc(digits[--n]);
}

_Noreturn void kernel_fail(const char *s)
{
    kernel_text("FAIL: ");
    kernel_text(s);
    kernel_text("\n");
    *(volatile uint32_t *)0x100000UL = 0x13333;
    for (;;)
        __asm__ volatile("wfi");
}

_Noreturn void kernel_finish(void)
{
#ifdef SERVICES
    kernel_text("M7: PASS\n");
#else
    kernel_text("M6: PASS\n");
#endif
    *(volatile uint32_t *)0x100000UL = 0x5555;
    for (;;)
        __asm__ volatile("wfi");
}

_Noreturn void kernel_fault(void)
{
    uint64_t cause, pc, address;
    __asm__ volatile("csrr %0, scause" : "=r"(cause));
    __asm__ volatile("csrr %0, sepc" : "=r"(pc));
    __asm__ volatile("csrr %0, stval" : "=r"(address));
    kernel_text("S trap cause=");
    kernel_number(cause);
    kernel_text(" pc=");
    kernel_number(pc);
    kernel_text(" address=");
    kernel_number(address);
    kernel_text("\n");
    kernel_fail("supervisor fault");
}

void grant(unsigned world, unsigned slot, enum Right right, unsigned object)
{
    if (world >= WORLD_COUNT || slot >= CAP_COUNT || right <= RIGHT_NONE ||
        right > RIGHT_CONSOLE)
        kernel_fail("invalid boot grant");
    if ((right == RIGHT_SEND || right == RIGHT_RECEIVE) && object >= ENDPOINT_COUNT)
        kernel_fail("invalid endpoint grant");
    if (threads[world].caps[slot].right != RIGHT_NONE ||
        (right == RIGHT_SEND && endpoints[object].sender >= 0) ||
        (right == RIGHT_RECEIVE && endpoints[object].receiver >= 0))
        kernel_fail("duplicate boot grant");
    threads[world].caps[slot] = (Capability){right, object};
    if (right == RIGHT_SEND)
        endpoints[object].sender = (int)world;
    if (right == RIGHT_RECEIVE)
        endpoints[object].receiver = (int)world;
}

#ifdef SERVICES
/* Boot-time authority: mappings are fixed, page-aligned, RW and non-executable.
 * Device policy and VirtIO register operations remain in user space. */
void grant_device(unsigned world, uintptr_t address)
{
    if (world >= WORLD_COUNT || (address != UART_BASE && address != BLOCK_BASE))
        kernel_fail("invalid device grant");
    devices[world][(address - UART_BASE) / PAGE] =
        pte(address, LEAF | PTE_U | PTE_R | PTE_W);
}

void grant_dma(unsigned world)
{
    if (world >= WORLD_COUNT)
        kernel_fail("invalid DMA grant");
    lower[world][DMA_VA >> 21] = pte((uintptr_t)dma_leaves[world], PTE_V);
    dma_leaves[world][(DMA_VA >> 12) & 511] =
        pte((uintptr_t)dma_page, LEAF | PTE_U | PTE_R | PTE_W);
    threads[world].frame.x[13] = (uintptr_t)dma_page;
}
#endif

void worlds_reset(unsigned scenario)
{
    __asm__ volatile("csrw satp, zero; sfence.vma zero, zero" ::: "memory");
    fresh = true;
    zero(threads, sizeof(threads));
    zero(roots, sizeof(roots));
    zero(lower, sizeof(lower));
    zero(leaves, sizeof(leaves));
    zero(memory, sizeof(memory));
    zero(devices, sizeof(devices));
#ifdef SERVICES
    /* Demo resets only after the driver has quiesced the device. */
    zero(dma_page, sizeof(dma_page));
    zero(dma_leaves, sizeof(dma_leaves));
#endif
    size_t image_size = (size_t)(user_image_end - user_image_start);
    if (image_size >= STACK_GUARD - USER_BASE || USER_RO_END <= USER_BASE ||
        USER_RO_END >= STACK_GUARD || (USER_RO_END & (PAGE - 1)))
        kernel_fail("invalid user image layout");
    for (unsigned e = 0; e < ENDPOINT_COUNT; ++e)
        endpoints[e] = (Endpoint){-1, -1, -1, -1, 0, false};
    for (unsigned id = 0; id < WORLD_COUNT; ++id) {
        for (size_t j = 0; j < image_size; ++j)
            memory[id][j] = user_image_start[j];
        /* Sv39: root[2] is a supervisor-only identity mapping of RAM.
         * The kernel never enables SUM, and never dereferences U pointers. */
        roots[id][2] = pte(0x80000000UL, LEAF | PTE_R | PTE_W | PTE_X);
        roots[id][0] = pte((uintptr_t)lower[id], PTE_V);
        lower[id][0] = pte((uintptr_t)test_device, PTE_V);
        lower[id][128] = pte((uintptr_t)devices[id], PTE_V);
        lower[id][2] = pte((uintptr_t)leaves[id], PTE_V);
        for (unsigned j = 0; j < 512; ++j) {
            uintptr_t va = USER_BASE + j * PAGE;
            if (va == STACK_GUARD)
                continue;
            uint64_t flags = LEAF | PTE_U | PTE_R;
            flags |= va < USER_RO_END ? PTE_X : PTE_W;
            leaves[id][j] = pte((uintptr_t)&memory[id][j * PAGE], flags);
        }
        devices[id][0] = pte(0x10000000UL, LEAF | PTE_R | PTE_W);
#ifdef SERVICES
        devices[id][16] = pte(0x10000000UL, LEAF | PTE_R | PTE_W);
#endif
        threads[id].state = RUNNABLE;
        threads[id].frame.pc = USER_ENTRY;
        threads[id].frame.x[2] = USER_TOP;
        threads[id].frame.x[10] = id;
        threads[id].frame.x[11] = scenario;
        threads[id].peer_physical = (uintptr_t)memory[(id + 1) % WORLD_COUNT];
        threads[id].frame.x[12] = threads[id].peer_physical;
    }
    current = WORLD_COUNT - 1;
    __asm__ volatile("fence.i" ::: "memory");
    __asm__ volatile("sfence.vma zero, zero" ::: "memory");
    __asm__ volatile("csrc sip, %0" ::"r"(2UL) : "memory");
}

void retire(unsigned world, enum State state)
{
    threads[world].state = state;
    for (unsigned e = 0; e < ENDPOINT_COUNT; ++e) {
        Endpoint *ep = &endpoints[e];
        if (ep->sender != (int)world && ep->receiver != (int)world)
            continue;
        ep->closed = true;
        int waiting[2] = {ep->waiting_sender, ep->waiting_receiver};
        for (unsigned i = 0; i < 2; ++i) {
            if (waiting[i] >= 0 && waiting[i] != (int)world) {
                Thread *t = &threads[waiting[i]];
                if (t->state == SENDING || t->state == RECEIVING) {
                    t->frame.x[10] = (uint64_t)(int64_t)IPC_PEER_GONE;
                    t->state = RUNNABLE;
                }
            }
        }
        ep->waiting_sender = ep->waiting_receiver = -1;
    }
}

static void ipc(Thread *caller, bool send)
{
    uint64_t slot = caller->frame.x[10];
    if (slot >= CAP_COUNT ||
        caller->caps[slot].right != (send ? RIGHT_SEND : RIGHT_RECEIVE)) {
        caller->frame.x[10] = (uint64_t)(int64_t)IPC_DENIED;
        return;
    }
    Endpoint *ep = &endpoints[caller->caps[slot].object];
    if (ep->closed || ep->sender < 0 || ep->receiver < 0) {
        caller->frame.x[10] = (uint64_t)(int64_t)IPC_PEER_GONE;
        return;
    }
    caller->frame.x[10] = IPC_OK;
    if (send) {
        if (ep->waiting_receiver >= 0) {
            Thread *receiver = &threads[ep->waiting_receiver];
            receiver->frame.x[10] = IPC_OK;
            receiver->frame.x[11] = caller->frame.x[11];
            receiver->state = RUNNABLE;
            ep->waiting_receiver = -1;
        } else {
            ep->value = caller->frame.x[11];
            ep->waiting_sender = (int)current;
            caller->state = SENDING;
        }
    } else {
        if (ep->waiting_sender >= 0) {
            caller->frame.x[11] = ep->value;
            threads[ep->waiting_sender].state = RUNNABLE;
            ep->waiting_sender = -1;
        } else {
            ep->waiting_receiver = (int)current;
            caller->state = RECEIVING;
        }
    }
}

static Frame *choose(bool rotate)
{
    if (fresh) {
        rotate = true;
        fresh = false;
    }
    if (rotate || threads[current].state != RUNNABLE) {
        unsigned next;
        for (next = 1; next <= WORLD_COUNT; ++next) {
            unsigned candidate = (current + next) % WORLD_COUNT;
            if (threads[candidate].state == RUNNABLE) {
                current = candidate;
                break;
            }
        }
        if (next > WORLD_COUNT)
            kernel_fail("no runnable worlds (deadlock)");
    }
    uintptr_t satp = (8UL << 60) | ((uintptr_t)roots[current] >> 12);
    __asm__ volatile("csrw satp, %0; sfence.vma zero, zero" ::"r"(satp) : "memory");
    return &threads[current].frame;
}

Frame *handle_trap(Frame *frame)
{
    uint64_t cause, address;
    __asm__ volatile("csrr %0, scause" : "=r"(cause));
    __asm__ volatile("csrr %0, stval" : "=r"(address));
    if (frame->status & 256)
        kernel_fail("unexpected S-mode trap");
    Thread *t = &threads[current];
    t->frame = *frame;
    bool rotate = false;
    if (cause == ((1UL << 63) | 1)) {
        __asm__ volatile("csrc sip, %0" ::"r"(2UL) : "memory");
        ++t->preemptions;
        t->last_preempt_pc = frame->pc;
        rotate = true;
    } else if (cause == 8) {
        t->frame.pc += 4; /* ECALL is always 32 bits, even with the C extension. */
        uint64_t op = t->frame.x[17];
        if (op == SYS_EXIT) {
            t->exit_code = t->frame.x[10];
            retire(current, EXITED);
        } else if (op == SYS_YIELD) {
            t->frame.x[10] = IPC_OK;
            rotate = true;
        } else if (op == SYS_SEND || op == SYS_RECEIVE) {
            ipc(t, op == SYS_SEND);
        } else if (op == SYS_PUTC) {
            uint64_t slot = t->frame.x[10];
            if (slot < CAP_COUNT && t->caps[slot].right == RIGHT_CONSOLE) {
                putc((char)t->frame.x[11]);
                t->frame.x[10] = IPC_OK;
            } else
                t->frame.x[10] = (uint64_t)(int64_t)IPC_DENIED;
        } else
            t->frame.x[10] = (uint64_t)(int64_t)SYS_UNKNOWN;
    } else {
        if (cause & (1UL << 63))
            kernel_fail("unexpected interrupt");
        t->fault = cause;
        t->fault_address = address;
        retire(current, FAULTED);
    }
    demo_progress();
    return choose(rotate);
}

_Noreturn void supervisor_main(void)
{
    __asm__ volatile("csrw sscratch, zero");
    __asm__ volatile("csrw stvec, %0" ::"r"(supervisor_trap));
    __asm__ volatile("csrw sie, %0" ::"r"(2UL));
    /* Only the UART and shutdown device need S-mode MMIO mappings. */
    test_device[256] = pte(0x100000UL, LEAF | PTE_R | PTE_W);
#ifdef SERVICES
    kernel_text("M7: supervisor kernel\n");
#else
    kernel_text("M6: supervisor kernel\n");
#endif
    demo_start();
    restore_user(choose(true));
}
