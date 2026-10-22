/* SPDX-License-Identifier: MPL-2.0 */
#include "services/user.h"
void block_service(unsigned scenario, uintptr_t physical)
{
    bool ready = false;
    for (;;) {
        Reply request = call(SYS_RECEIVE, 1, 0);
        if (request.status == IPC_PEER_GONE)
            break;
        require(request.status == IPC_OK, 220);
        if (scenario == BLOCK_FAULT || scenario == BLOCK_UART) {
            uintptr_t address = scenario == BLOCK_UART ? UART_BASE : 0x80000000UL;
            (void)*(volatile uint64_t *)address;
            finish(221);
        }
        unsigned op = message_op(request.value), id = message_id(request.value);
        uint32_t sector = message_arg(request.value);
        if (op == OP_STOP && sector == 0) {
            if (ready) {
                block_close();
                ready = false;
            }
            call(SYS_SEND, 2, message(op, id, SERVICE_OK));
            break;
        }
        unsigned status = SERVICE_BAD_REQUEST;
        uint64_t data[64];
        if (op == OP_READ) {
            if (!ready) {
                ready = block_init(physical);
                if (ready)
                    require(block_error_probe(), 223);
            }
            status = ready ? block_read(sector, data) : SERVICE_IO;
        }
        if (call(SYS_SEND, 2, message(op, id, status)).status == IPC_PEER_GONE)
            break;
        if (status == SERVICE_OK) {
            for (unsigned i = 0; i < 64; ++i)
                if (call(SYS_SEND, 2, data[i]).status == IPC_PEER_GONE)
                    goto closed;
        }
    }
closed:
    if (ready)
        block_close();
}
