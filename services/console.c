/* SPDX-License-Identifier: MPL-2.0 */
#include "services/user.h"
void console_service(unsigned scenario)
{
    for (;;) {
        Reply request = call(SYS_RECEIVE, 1, 0);
        if (request.status == IPC_PEER_GONE)
            return;
        require(request.status == IPC_OK, 200);
        if (scenario == CONSOLE_FAULT || scenario == CONSOLE_BLOCK) {
            uintptr_t address = scenario == CONSOLE_BLOCK ? BLOCK_BASE : 0x80000000UL;
            (void)*(volatile uint64_t *)address;
            finish(201);
        }
        unsigned op = message_op(request.value), id = message_id(request.value);
        uint32_t length = message_arg(request.value);
        unsigned status = SERVICE_OK;
        if (op == OP_STOP && length == 0) {
            call(SYS_SEND, 2, message(op, id, SERVICE_OK));
            return;
        }
        if (op != OP_CONSOLE || length == 0 || length > 512)
            status = SERVICE_BAD_REQUEST;
        else {
            /* Snapshot complete bounded payload before touching the device. */
            unsigned char bytes[512];
            for (unsigned i = 0; i < length; i += 8) {
                Reply r = call(SYS_RECEIVE, 1, 0);
                if (r.status == IPC_PEER_GONE)
                    return;
                require(r.status == IPC_OK, 202);
                for (unsigned j = 0; j < 8 && i + j < length; ++j)
                    bytes[i + j] = r.value >> (8 * j);
            }
            volatile uint8_t *uart = (void *)UART_BASE;
            for (unsigned i = 0; i < length; ++i) {
                unsigned n;
                for (n = 0; n < 100000; ++n) {
                    if (uart[5] & 32)
                        break;
                    call(SYS_YIELD, 0, 0);
                }
                require(n < 100000, 203);
                uart[0] = bytes[i];
            }
        }
        if (call(SYS_SEND, 2, message(op, id, status)).status == IPC_PEER_GONE)
            return;
    }
}
