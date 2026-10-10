/* SPDX-License-Identifier: MPL-2.0 */

#include "platform/hosted/platform.h"

#include <stdlib.h>

static void *host_allocate(void *context, size_t size)
{
    (void)context;
    return malloc(size);
}

static void host_deallocate(void *context, void *pointer)
{
    (void)context;
    free(pointer);
}

VmPlatform hosted_platform(void)
{
    VmPlatform platform = {NULL, host_allocate, host_deallocate};
    return platform;
}
