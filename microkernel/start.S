/* SPDX-License-Identifier: MPL-2.0 */
.section .text.boot
.globl _start
.align 2
_start:
    csrr t0, mhartid
    bnez t0, .Lpark
    la sp, __mstack_top
    la t0, __bss_start
    la t1, __bss_end
.Lzero:
    bgeu t0, t1, .Lsetup
    sd zero, 0(t0)
    addi t0, t0, 8
    j .Lzero
.Lsetup:
    /* The S kernel owns physical memory. U access is restricted by Sv39. */
    li t0, -1
    csrw pmpaddr0, t0
    li t0, 0x1f
    csrw pmpcfg0, t0
    csrw satp, zero
    csrw mcounteren, zero
    csrw scounteren, zero
    li t0, 0xb1ff
    csrw medeleg, t0
    li t0, 2
    csrw mideleg, t0
    la t0, machine_trap
    csrw mtvec, t0
    la t0, machine_scratch
    csrw mscratch, t0
    li t0, 0x200bff8
    ld t1, 0(t0)
    li t2, 100000
    add t1, t1, t2
    li t0, 0x2004000
    sd t1, 0(t0)
    li t0, 128
    csrw mie, t0
    /* MPP=S, MPIE=1. mret enables machine timer interrupts. */
    li t0, 0xa00000880 /* SXL=UXL=64 */
    csrw mstatus, t0
    la t0, supervisor_main
    csrw mepc, t0
    la sp, __sstack_top
    mret
.Lpark:
    wfi
    j .Lpark

/* M-mode timer relay, independent of user registers and all S/U stacks. */
.align 2
machine_trap:
    csrrw t0, mscratch, t0
    sd t1, 8(t0)
    sd t2, 16(t0)
    sd t3, 24(t0)
    csrr t1, mcause
    li t2, 0x8000000000000007
    bne t1, t2, .Lmachine_fail
    li t1, 0x200bff8
    ld t2, 0(t1)
    li t3, 100000
    add t2, t2, t3
    li t1, 0x2004000
    sd t2, 0(t1)
    li t1, 2
    csrs mip, t1
    ld t1, 8(t0)
    ld t2, 16(t0)
    ld t3, 24(t0)
    csrrw t0, mscratch, t0
    mret
.Lmachine_fail:
    li t1, 0x100000
    li t2, 0x13333
    sw t2, 0(t1)
    j .Lpark
.section .bss
.align 4
machine_scratch: .skip 32
