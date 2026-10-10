#include "platform/riscv64/platform.h"

static void write_text(const char *text)
{
    size_t length = 0;

    while (text[length] != '\0') {
        ++length;
    }
    riscv64_console_write(NULL, text, length);
}

_Noreturn void kernel_main(void)
{
    static char line[512];
    Vm vm;

    write_text("Lisp machine booted.\r\n");
    if (!vm_init(&vm, riscv64_platform())) {
        write_text("fatal: ");
        write_text(vm_error(&vm));
        write_text("\r\n");
        riscv64_qemu_exit();
    }

    for (;;) {
        VmObject *result;
        Riscv64LineResult line_result;

        write_text("lisp> ");
        line_result = riscv64_console_read_line(line, sizeof(line));
        if (line_result == RISCV64_LINE_END) {
            break;
        }
        if (line_result == RISCV64_LINE_TOO_LONG) {
            write_text("error: input exceeds 511 characters\r\n");
            continue;
        }
        if (line[0] == '\0') {
            continue;
        }
        result = vm_eval_source(&vm, line);
        if (result == NULL) {
            write_text("error: ");
            write_text(vm_error(&vm));
            write_text("\r\n");
            continue;
        }
        vm_print(result, riscv64_console_write, NULL);
        write_text("\r\n");
    }

    vm_destroy(&vm);
    riscv64_qemu_exit();
}
