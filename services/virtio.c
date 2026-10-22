/* SPDX-License-Identifier: MPL-2.0 */
/* Modern VirtIO MMIO, one split queue, one outstanding read, little-endian RV64.
 * Runs entirely in U mode. Only this trusted driver knows DMA addresses. */
#include "services/user.h"
struct Descriptor {
    uint64_t address;
    uint32_t length;
    uint16_t flags, next;
};
_Static_assert(sizeof(struct Descriptor) == 16, "VirtIO descriptor layout");
static uintptr_t dma_physical;
static uint16_t submitted;
static uint64_t capacity;
static volatile uint32_t *const mmio = (void *)BLOCK_BASE;
static void barrier(void)
{
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}
static uint32_t read_reg(unsigned offset)
{
    uint32_t v = mmio[offset / 4];
    barrier();
    return v;
}
static void write_reg(unsigned offset, uint32_t value)
{
    barrier();
    mmio[offset / 4] = value;
    barrier();
}
static void address_reg(unsigned offset, uintptr_t address)
{
    write_reg(offset, (uint32_t)address);
    write_reg(offset + 4, address >> 32);
}
void block_close(void)
{
    write_reg(0x70, 0);
    for (unsigned n = 0; n < 100000; ++n)
        if (!read_reg(0x70))
            return;
    finish(310); /* Do not reuse DMA storage after a failed reset. */
}
bool block_init(uintptr_t physical)
{
    if (read_reg(0) != 0x74726976 || read_reg(4) != 2 || read_reg(8) != 2)
        return false;
    block_close();
    dma_physical = physical;
    submitted = 0;
    volatile uint8_t *page = (void *)DMA_VA;
    for (unsigned n = 0; n < 4096; ++n)
        page[n] = 0;
    write_reg(0x70, 1);
    write_reg(0x70, 3);
    write_reg(0x14, 1);
    if (!(read_reg(0x10) & 1))
        goto failed; /* VERSION_1 */
    write_reg(0x24, 0);
    write_reg(0x20, 0);
    write_reg(0x24, 1);
    write_reg(0x20, 1);
    write_reg(0x70, 11);
    if (!(read_reg(0x70) & 8))
        goto failed;
    write_reg(0x30, 0);
    if (read_reg(0x44) || read_reg(0x34) < 8)
        goto failed;
    write_reg(0x38, 8);
    address_reg(0x80, physical);
    address_reg(0x90, physical + 0x100);
    address_reg(0xa0, physical + 0x200);
    write_reg(0x44, 1);
    /* Static test disk; still honour configuration generation for 64-bit reads. */
    bool stable = false;
    for (unsigned n = 0; n < 1000; ++n) {
        uint32_t generation = read_reg(0xfc);
        uint32_t lo = read_reg(0x100), hi = read_reg(0x104);
        if (generation == read_reg(0xfc)) {
            capacity = (uint64_t)hi << 32 | lo;
            stable = true;
            break;
        }
    }
    if (!stable)
        goto failed;
    write_reg(0x70, 15);
    return true;
failed:
    block_close();
    return false;
}
static unsigned transfer(uint32_t type, uint64_t sector, uint64_t output[64])
{
    volatile struct Descriptor *desc = (void *)DMA_VA;
    volatile uint16_t *avail = (void *)(DMA_VA + 0x100);
    volatile uint16_t *used = (void *)(DMA_VA + 0x200);
    volatile uint32_t *header = (void *)(DMA_VA + 0x300);
    volatile uint8_t *status = (void *)(DMA_VA + 0x600);
    header[0] = type;
    header[1] = 0;
    *(volatile uint64_t *)(DMA_VA + 0x308) = sector;
    *status = 255;
    desc[0] = (struct Descriptor){dma_physical + 0x300, 16, 1, 1};
    desc[1] = (struct Descriptor){dma_physical + 0x400, 512, 3, 2};
    desc[2] = (struct Descriptor){dma_physical + 0x600, 1, 2, 0};
    avail[0] = 1; /* NO_INTERRUPT: completion is polled with cooperative yields. */
    unsigned slot = submitted % 8;
    avail[2 + slot] = 0;
    barrier();
    avail[1] = ++submitted;
    barrier();
    write_reg(0x50, 0);
    unsigned n;
    for (n = 0; n < 100000; ++n) {
        if (used[1] == submitted)
            break;
        call(SYS_YIELD, 0, 0);
    }
    if (n == 100000) {
        block_close();
        finish(311);
    }
    barrier();
    volatile uint32_t *entry = (void *)(DMA_VA + 0x204 + 8 * slot);
    require(entry[0] == 0 && entry[1] >= 1 && entry[1] <= 513, 312);
    unsigned result = *status;
    if (!result) {
        require(entry[1] == 513, 313);
        volatile uint64_t *data = (void *)(DMA_VA + 0x400);
        for (unsigned i = 0; i < 64; ++i)
            output[i] = data[i];
    }
    write_reg(0x64, read_reg(0x60));
    return result;
}
unsigned block_read(uint64_t sector, uint64_t output[64])
{
    if (sector >= capacity)
        return SERVICE_RANGE;
    return transfer(0, sector, output) == 0 ? SERVICE_OK : SERVICE_IO;
}
bool block_error_probe(void)
{
    uint64_t ignored[64];
    return transfer(0x7fffffff, 0, ignored) == 2; /* UNSUPP from the actual device */
}
