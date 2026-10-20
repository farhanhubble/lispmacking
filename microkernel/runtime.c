/* SPDX-License-Identifier: MPL-2.0 */
#include <stddef.h>
/* Freestanding compiler-generated aggregate copies and clears. */
void *memset(void *dest, int c, size_t n)
{
    unsigned char *p = dest;
    for (size_t i = 0; i < n; ++i)
        p[i] = (unsigned char)c;
    return dest;
}
void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; ++i)
        d[i] = s[i];
    return dest;
}
