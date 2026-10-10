/* SPDX-License-Identifier: MPL-2.0 */

#include "platform/hosted/platform.h"

#include <stdio.h>

static void file_write(void *context, const char *bytes, size_t length)
{
    (void)fwrite(bytes, 1, length, context);
}

int main(void)
{
    char line[4096];
    Vm vm;

    if (!vm_init(&vm, hosted_platform())) {
        (void)fprintf(stderr, "failed to initialize VM: %s\n", vm_error(&vm));
        vm_destroy(&vm);
        return 1;
    }

    for (;;) {
        VmObject *result;

        (void)fputs("lisp> ", stdout);
        (void)fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) {
            (void)fputc('\n', stdout);
            break;
        }
        result = vm_eval_source(&vm, line);
        if (result == NULL) {
            if (vm_error(&vm)[0] != '\0') {
                (void)fprintf(stderr, "error: %s\n", vm_error(&vm));
            }
            continue;
        }
        vm_print(result, file_write, stdout);
        (void)fputc('\n', stdout);
    }

    vm_destroy(&vm);
    return 0;
}
