/* SPDX-License-Identifier: MPL-2.0 */
#include "services/user.h"
#include "vm/internal.h"
extern unsigned char __user_heap_start[];
static uintptr_t heap;
static unsigned next_id;
static void *allocate(void *context, size_t size)
{
    (void)context;
    if (size > SIZE_MAX - 15)
        return NULL;
    size = (size + 15) & ~(size_t)15;
    if (heap > STACK_GUARD || size > STACK_GUARD - heap)
        return NULL;
    void *p = (void *)heap;
    heap += size;
    return p;
}
static void deallocate(void *context, void *p)
{
    (void)context;
    (void)p;
}
static uint64_t request(unsigned slot, unsigned op, uint32_t argument)
{
    uint64_t m = message(op, ++next_id, argument);
    require(call(SYS_SEND, slot, m).status == IPC_OK, 100);
    return m;
}
static unsigned completion(unsigned slot, uint64_t m)
{
    Reply r = call(SYS_RECEIVE, slot, 0);
    require(r.status == IPC_OK, 101);
    require(message_op(r.value) == message_op(m) &&
                message_id(r.value) == message_id(m),
            102);
    return message_arg(r.value);
}
static void output(void *context, const char *text, size_t length)
{
    (void)context;
    while (length) {
        unsigned n = length > 512 ? 512 : (unsigned)length;
        uint64_t m = request(3, OP_CONSOLE, n);
        for (unsigned i = 0; i < n; i += 8) {
            uint64_t word = 0;
            for (unsigned j = 0; j < 8 && i + j < n; ++j)
                word |= (uint64_t)(unsigned char)text[i + j] << (8 * j);
            require(call(SYS_SEND, 3, word).status == IPC_OK, 103);
        }
        require(completion(4, m) == SERVICE_OK, 104);
        text += n;
        length -= n;
    }
}
static void text(const char *s)
{
    size_t n = 0;
    while (s[n])
        ++n;
    output(NULL, s, n);
}
static void read_sector(uint32_t sector, unsigned char *buffer)
{
    uint64_t m = request(1, OP_READ, sector);
    require(completion(2, m) == SERVICE_OK, 105);
    for (unsigned i = 0; i < 64; ++i) {
        Reply r = call(SYS_RECEIVE, 2, 0);
        require(r.status == IPC_OK, 106);
        for (unsigned j = 0; j < 8; ++j)
            buffer[i * 8 + j] = r.value >> (8 * j);
    }
}
static void arithmetic(void)
{
    Vm vm;
    VmPlatform platform = {0};
    heap = (uintptr_t)__user_heap_start;
    platform.allocate = allocate;
    platform.deallocate = deallocate;
    require(vm_init(&vm, platform), 107);
    VmObject *r = vm_eval_source(&vm, "(+ 40 2)");
    require(r && r->type == VM_INTEGER && r->as.integer == 42, 108);
}
static void client(unsigned scenario)
{
    require(call(SYS_PUTC, 0, '!').status == IPC_DENIED, 109);
    require(call(SYS_SEND, 4, 0).status == IPC_DENIED, 110);
    require(call(SYS_RECEIVE, 1, 0).status == IPC_DENIED, 111);
    require(call(SYS_SEND, UINT64_MAX, 0).status == IPC_DENIED, 112);
    if (scenario >= CLIENT_UART && scenario <= CLIENT_DMA) {
        uintptr_t address = scenario == CLIENT_UART    ? UART_BASE
                            : scenario == CLIENT_BLOCK ? BLOCK_BASE
                                                       : DMA_VA;
        (void)*(volatile uint64_t *)address;
        finish(113);
    }
    if (scenario == CLIENT_KERNEL_UART) {
        (void)*(volatile uint64_t *)0x10010000UL;
        finish(128);
    }
    if (scenario == CLIENT_BLOCK_EXIT) {
        uint64_t m = request(1, OP_READ, 1);
        require(completion(2, m) == SERVICE_OK, 129);
        require(call(SYS_RECEIVE, 2, 0).status == IPC_OK, 130);
        return;
    }
    if (scenario == CLIENT_EXIT) {
        /* Exit halfway through a valid console payload: no partial output. */
        request(3, OP_CONSOLE, 16);
        require(call(SYS_SEND, 3, 0x4141414141414141UL).status == IPC_OK, 114);
        return;
    }
    if (scenario == CONSOLE_FAULT || scenario == BLOCK_FAULT ||
        scenario == CONSOLE_BLOCK || scenario == BLOCK_UART) {
        bool console = scenario == CONSOLE_FAULT || scenario == CONSOLE_BLOCK;
        unsigned send = console ? 3 : 1, recv = send + 1;
        request(send, console ? OP_CONSOLE : OP_READ, 1);
        require(call(SYS_RECEIVE, recv, 0).status == IPC_PEER_GONE, 115);
        require(call(SYS_SEND, send, 0).status == IPC_PEER_GONE, 116);
        arithmetic();
        if (scenario == BLOCK_FAULT)
            text("client survived block fault\n");
        return;
    }
    uint64_t m = request(1, 255, 0);
    require(completion(2, m) == SERVICE_BAD_REQUEST, 117);
    m = request(3, OP_CONSOLE, 513);
    require(completion(4, m) == SERVICE_BAD_REQUEST, 118);
    m = request(3, OP_CONSOLE, 0);
    require(completion(4, m) == SERVICE_BAD_REQUEST, 119);
    m = request(1, OP_READ, UINT32_MAX);
    unsigned status = completion(2, m);
    if (status == SERVICE_IO) {
        text("block service reported I/O error\n");
        finish(131);
    }
    require(status == SERVICE_RANGE, 120);
    /* Exercise queue slot reuse and verify every byte of a known disk sector. */
    unsigned char buffer[513];
    for (unsigned round = 0; round < 10; ++round) {
        read_sector(1, buffer);
        for (unsigned i = 0; i < 512; ++i)
            require(buffer[i] == (unsigned char)(i * 37 + 11), 121);
    }
    text("services: bounds, grants, device error and disk bytes verified\n");
    read_sector(0, buffer);
    buffer[512] = 0;
    bool terminated = false;
    for (unsigned i = 0; i < 512; ++i)
        if (!buffer[i])
            terminated = true;
    require(terminated, 122);
    Vm vm;
    VmPlatform platform = {0};
    heap = (uintptr_t)__user_heap_start;
    platform.allocate = allocate;
    platform.deallocate = deallocate;
    require(vm_init(&vm, platform), 123);
    require(vm_eval_source(&vm, "(bootstrap:page-allocate)") == NULL, 124);
    VmObject *result = vm_eval_source(&vm, (const char *)buffer);
    require(result && result->type == VM_INTEGER && result->as.integer == 42, 125);
    text("disk Lisp result: ");
    vm_print(result, output, NULL);
    text("\n");
    m = request(1, OP_STOP, 0);
    require(completion(2, m) == SERVICE_OK, 126);
    m = request(3, OP_STOP, 0);
    require(completion(4, m) == SERVICE_OK, 127);
}
_Noreturn void user_main(uint64_t world, uint64_t scenario, uintptr_t peer,
                         uintptr_t physical)
{
    (void)peer;
    if (world == 0)
        client((unsigned)scenario);
    else if (world == 1)
        console_service((unsigned)scenario);
    else
        block_service((unsigned)scenario, physical);
    /* Every surviving service still evaluates Lisp after client/service failure. */
    arithmetic();
    finish(0);
}
