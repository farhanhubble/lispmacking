/* SPDX-License-Identifier: MPL-2.0 */
#ifndef SERVICES_PROTOCOL_H
#define SERVICES_PROTOCOL_H
#include <stdint.h>
#define UART_BASE 0x10000000UL
#define BLOCK_BASE 0x10008000UL
#define DMA_VA 0x700000UL
#define SECTOR_BYTES 512
/* Client slots: block request/reply 1/2, console request/reply 3/4.
 * Each server uses slots 1/2. Requests have op:8, id:24, argument:32.
 * Replies echo id and op, with status in argument. Successful block replies
 * are followed by exactly 64 words, copied into the client's private buffer.
 * One request outstanding per endpoint; no addresses cross the protocol. */
enum { OP_READ = 1, OP_CONSOLE = 2, OP_STOP = 3 };
enum { SERVICE_OK, SERVICE_BAD_REQUEST, SERVICE_RANGE, SERVICE_IO };
enum {
    SERVICE_DEMO,
    CLIENT_UART,
    CLIENT_BLOCK,
    CLIENT_DMA,
    CONSOLE_FAULT,
    BLOCK_FAULT,
    CLIENT_EXIT,
    CLIENT_BLOCK_EXIT,
    CONSOLE_BLOCK,
    BLOCK_UART,
    CLIENT_KERNEL_UART,
    SERVICE_CASES
};
static inline uint64_t message(unsigned op, unsigned id, uint32_t arg)
{
    return (uint64_t)arg << 32 | (uint64_t)(id & 0xffffff) << 8 | (op & 255);
}
static inline unsigned message_op(uint64_t m)
{
    return m & 255;
}
static inline unsigned message_id(uint64_t m)
{
    return (m >> 8) & 0xffffff;
}
static inline uint32_t message_arg(uint64_t m)
{
    return m >> 32;
}
#endif
