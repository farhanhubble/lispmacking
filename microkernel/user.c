/* SPDX-License-Identifier: MPL-2.0 */
/* The same portable evaluator executes entirely in U mode in each world. */
#include "build/microkernel/fixtures.h"
#include "microkernel/abi.h"
#include "vm/internal.h"

extern unsigned char __user_heap_start[];
static uintptr_t heap;
static volatile uint64_t private_word;
static _Alignas(4) volatile uint32_t data_instruction;

typedef struct {
    int64_t status;
    uint64_t value;
} Reply;
static Reply call(uint64_t op, uint64_t cap, uint64_t value)
{
    register uint64_t a0 __asm__("a0") = cap;
    register uint64_t a1 __asm__("a1") = value;
    register uint64_t a7 __asm__("a7") = op;
    __asm__ volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a7) : "memory");
    return (Reply){(int64_t)a0, a1};
}

static _Noreturn void finish(uint64_t status)
{
    (void)call(SYS_EXIT, status, 0);
    for (;;) {
    }
}

static void require(bool condition, uint64_t code)
{
    if (!condition)
        finish(code);
}

static void *allocate(void *context, size_t size)
{
    (void)context;
    if (size > SIZE_MAX - 15)
        return NULL;
    size = (size + 15) & ~(size_t)15;
    if (heap > STACK_GUARD || size > STACK_GUARD - heap)
        return NULL;
    void *result = (void *)heap;
    heap += size;
    return result;
}

static void deallocate(void *context, void *p)
{
    (void)context;
    (void)p;
}

static void print(void *context, const char *bytes, size_t length)
{
    (void)context;
    for (size_t i = 0; i < length; ++i)
        (void)call(SYS_PUTC, 0, (unsigned char)bytes[i]);
}

static void text(const char *s)
{
    size_t n = 0;
    while (s[n])
        ++n;
    print(NULL, s, n);
}

static bool arguments(Vm *vm, VmObject *args, unsigned count, int64_t *values)
{
    for (unsigned i = 0; i < count; ++i) {
        if (args->type != VM_CONS || args->as.cons.car->type != VM_INTEGER) {
            vm_set_error(vm, "IPC expects integer arguments");
            return false;
        }
        values[i] = args->as.cons.car->as.integer;
        args = args->as.cons.cdr;
    }
    if (args != vm->nil) {
        vm_set_error(vm, "wrong IPC argument count");
        return false;
    }
    return true;
}

static VmObject *ipc_send(Vm *vm, VmObject *args)
{
    int64_t values[2];
    if (!arguments(vm, args, 2, values))
        return NULL;
    Reply r = call(SYS_SEND, (uint64_t)values[0], (uint64_t)values[1]);
    if (r.status) {
        vm_set_error(vm, "IPC send denied or peer gone");
        return NULL;
    }
    return vm_true(vm);
}

static VmObject *ipc_receive(Vm *vm, VmObject *args)
{
    int64_t values[1];
    if (!arguments(vm, args, 1, values))
        return NULL;
    Reply r = call(SYS_RECEIVE, (uint64_t)values[0], 0);
    if (r.status) {
        vm_set_error(vm, "IPC receive denied or peer gone");
        return NULL;
    }
    return vm_integer(vm, (int64_t)r.value);
}

static void check_language(unsigned world)
{
    Vm vm;
    VmPlatform platform = {0};
    heap = (uintptr_t)__user_heap_start;
    platform.allocate = allocate;
    platform.deallocate = deallocate;
    require(vm_init(&vm, platform), 101);
    require(vm_bind_primitive(&vm, "ipc:send", ipc_send), 102);
    require(vm_bind_primitive(&vm, "ipc:receive", ipc_receive), 103);
    /* Installing names conveys no bootstrap authority to these adapters. */
    require(vm_eval_source(&vm, "(bootstrap:page-allocate)") == NULL, 104);
    require(vm_eval_source(&vm, "(ipc:send 3 42)") == NULL, 105);
    require(vm_eval_source(&vm, "(ipc:receive -1)") == NULL, 106);
    require(vm_eval_source(&vm, "(ipc:send 'bad 42)") == NULL, 107);
    require(vm_eval_source(&vm, "(ipc:receive 1 2)") == NULL, 108);
    const char *source = world ? receiver_source : sender_source;
    VmObject *result = NULL;
    while (*source) {
        VmObject *expression = vm_read(&vm, source, &source);
        if (!expression)
            break;
        result = vm_eval(&vm, expression, vm.global_environment);
        if (!result)
            break;
    }
    if (!result) {
        text(vm_error(&vm));
        text("\n");
        finish(109);
    }
    require(result == vm_true(&vm), 110);
    text(world ? "world B: Lisp IPC result " : "world A: Lisp IPC result ");
    vm_print(result, print, NULL);
    text("\n");
    vm_destroy(&vm);
}

static void live_peer(void)
{
    Vm vm;
    VmPlatform platform = {0};
    heap = (uintptr_t)__user_heap_start;
    platform.allocate = allocate;
    platform.deallocate = deallocate;
    require(vm_init(&vm, platform), 111);
    VmObject *result = vm_eval_source(&vm, "(+ 40 2)");
    require(result && result->type == VM_INTEGER && result->as.integer == 42, 112);
}

_Noreturn void user_main(uint64_t world, uint64_t scenario, uintptr_t peer_physical)
{
    /* Identical virtual address, different physical storage in both worlds. */
    private_word = world ? 0xbb : 0xaa;
    if (scenario == CASE_IPC) {
        require(call(999, 0, 0).status == SYS_UNKNOWN, 10);
        require(call(SYS_SEND, UINT64_MAX, 0).status == IPC_DENIED, 11);
        require(call(SYS_RECEIVE, 3, 0).status == IPC_DENIED, 12);
        require(call(SYS_SEND, world ? 1 : 2, 0).status == IPC_DENIED, 13);
        require(call(SYS_RECEIVE, world ? 2 : 1, 0).status == IPC_DENIED, 14);
        require(call(SYS_PUTC, 1, '!').status == IPC_DENIED, 15);
        require(call(SYS_YIELD, 0, 0).status == IPC_OK, 28);
        check_language((unsigned)world);
        /* A second round tests full-width payloads and both rendezvous orders. */
        if (!world) {
            require(call(SYS_SEND, 1, 0x8000000000000000UL).status == IPC_OK, 16);
            Reply r = call(SYS_RECEIVE, 2, 0);
            require(r.status == IPC_OK && r.value == 0xffffffffffffffffUL, 17);
        } else {
            Reply r = call(SYS_RECEIVE, 1, 0);
            require(r.status == IPC_OK && r.value == 0x8000000000000000UL, 18);
            require(call(SYS_SEND, 2, 0xffffffffffffffffUL).status == IPC_OK, 19);
        }
        require(private_word == (world ? 0xbbUL : 0xaaUL), 20);
        finish(0);
    }
    if (scenario == CASE_PREEMPT) {
        if (!world) {
            /* The kernel checks that a timer actually interrupts this loop. */
            __asm__ volatile(".globl user_spin_begin, user_spin_end\n"
                             "user_spin_begin: j user_spin_begin\n"
                             "user_spin_end:\n" ::
                                 : "memory");
            __builtin_unreachable();
        }
        live_peer();
        finish(0);
    }
    if (scenario == CASE_PEER_EXIT) {
        if (!world) {
            /* Rendezvous first makes the peer live before testing teardown. */
            require(call(SYS_SEND, 1, 42).status == IPC_OK, 25);
            require(call(SYS_RECEIVE, 2, 0).status == IPC_PEER_GONE, 26);
        } else
            require(call(SYS_RECEIVE, 1, 0).status == IPC_OK, 27);
        finish(0);
    }
    if (scenario == CASE_NO_ENDPOINT) {
        require(call(world ? SYS_RECEIVE : SYS_SEND, 1, 42).status == IPC_DENIED, 29);
        live_peer();
        finish(0);
    }
    if (scenario == CASE_PEER_FAULT) {
        if (!world) {
            require(call(SYS_SEND, 1, 42).status == IPC_PEER_GONE, 30);
            require(call(SYS_SEND, 1, 42).status == IPC_PEER_GONE, 31);
            live_peer();
            finish(0);
        }
        private_word = *(volatile uint64_t *)0x80000000UL;
        finish(201);
    }
    if (world) {
        live_peer();
        finish(0);
    }
    if (scenario == CASE_KERNEL_READ)
        private_word = *(volatile uint64_t *)0x80000000UL;
    else if (scenario == CASE_PEER_READ)
        private_word = *(volatile uint64_t *)peer_physical;
    else if (scenario == CASE_UART_READ)
        private_word = *(volatile uint8_t *)0x10000000UL;
    else if (scenario == CASE_TEXT_WRITE)
        *(volatile uint32_t *)USER_BASE = 0;
    else if (scenario == CASE_DATA_EXEC) {
        data_instruction =
            0x00008067; /* ret, if executing writable data were allowed */
        __asm__ volatile("fence.i" ::: "memory");
        ((void (*)(void))(uintptr_t)&data_instruction)();
    } else if (scenario == CASE_CSR)
        __asm__ volatile("csrw satp, zero" ::: "memory");
    else if (scenario == CASE_GUARD_READ)
        private_word = *(volatile uint64_t *)STACK_GUARD;
    else if (scenario == CASE_KERNEL_EXEC)
        ((void (*)(void))0x80000000UL)();
    finish(200); /* Reaching this means the forbidden operation did not trap. */
}
