/* SPDX-License-Identifier: MPL-2.0 */
.section .rodata
.balign 16
.globl user_image_start, user_image_end
user_image_start:
#ifndef USER_IMAGE
#define USER_IMAGE "build/microkernel/user.bin"
#endif
.incbin USER_IMAGE
user_image_end:
