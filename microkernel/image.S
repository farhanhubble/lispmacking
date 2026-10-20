/* SPDX-License-Identifier: MPL-2.0 */
.section .rodata
.balign 16
.globl user_image_start, user_image_end
user_image_start:
.incbin "build/microkernel/user.bin"
user_image_end:
