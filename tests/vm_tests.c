/* SPDX-License-Identifier: MPL-2.0 */

#include "platform/hosted/platform.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char bytes[512];
    size_t length;
} Buffer;

static int failures;

static void buffer_write(void *context, const char *bytes, size_t length)
{
    Buffer *buffer = context;
    size_t available = sizeof(buffer->bytes) - buffer->length - 1;

    if (length > available) {
        length = available;
    }
    (void)memcpy(buffer->bytes + buffer->length, bytes, length);
    buffer->length += length;
    buffer->bytes[buffer->length] = '\0';
}

static void check(bool condition, const char *name)
{
    if (!condition) {
        (void)fprintf(stderr, "FAIL: %s\n", name);
        ++failures;
    }
}

static void expect_eval(Vm *vm, const char *source, const char *expected)
{
    VmObject *result = vm_eval_source(vm, source);
    Buffer buffer = {{0}, 0};

    if (result == NULL) {
        (void)fprintf(stderr, "FAIL: %s -> error: %s\n", source,
                      vm_error(vm));
        ++failures;
        return;
    }
    vm_print(result, buffer_write, &buffer);
    if (strcmp(buffer.bytes, expected) != 0) {
        (void)fprintf(stderr, "FAIL: %s -> %s (expected %s)\n", source,
                      buffer.bytes, expected);
        ++failures;
    }
}

static void expect_error(Vm *vm, const char *source)
{
    VmObject *result = vm_eval_source(vm, source);

    if (result != NULL || vm_error(vm)[0] == '\0') {
        (void)fprintf(stderr, "FAIL: %s should fail\n", source);
        ++failures;
    }
}

int main(void)
{
    Vm vm;
    VmObject *first_symbol;
    VmObject *second_symbol;
    const char *end;

    check(vm_init(&vm, hosted_platform()), "VM initializes");

    first_symbol = vm_intern(&vm, "same");
    second_symbol = vm_intern(&vm, "same");
    check(first_symbol == second_symbol, "symbols are interned");
    check(vm_read(&vm, "nil", &end) == vm_nil(&vm), "nil is a singleton");
    check(*end == '\0', "reader reports the end position");

    expect_eval(&vm, "'alpha", "alpha");
    expect_eval(&vm, "'(a b . c)", "(a b . c)");
    expect_eval(&vm, "(atom 'a)", "t");
    expect_eval(&vm, "(atom '(a))", "nil");
    expect_eval(&vm, "(eq 'a 'a)", "t");
    expect_eval(&vm, "(eq 'a 'b)", "nil");
    expect_eval(&vm, "(car '(a b))", "a");
    expect_eval(&vm, "(cdr '(a b))", "(b)");
    expect_eval(&vm, "(cons 'a '(b c))", "(a b c)");
    expect_eval(&vm, "(cond ((eq 'a 'b) 'wrong) (t 'right))", "right");
    expect_eval(&vm, "(cond ((eq 'a 'b) 'wrong))", "nil");
    expect_eval(&vm, "((lambda (x) (cons x '(b))) 'a)", "(a b)");
    expect_eval(&vm,
                "(((lambda (x) (lambda (y) (cons x (cons y nil)))) 'a) 'b)",
                "(a b)");
    expect_eval(&vm,
                "((lambda (x) ((lambda (x) x) 'inner)) 'outer)",
                "inner");
    expect_eval(&vm, "; comment\n(cons 'a nil)", "(a)");

    expect_error(&vm, "missing");
    expect_error(&vm, "(car 'a)");
    expect_error(&vm, "((lambda (x) x) 'a 'b)");
    expect_error(&vm, "(quote a b)");
    expect_error(&vm, "(a");
    expect_error(&vm, "'a 'b");

    vm_destroy(&vm);
    if (failures != 0) {
        (void)fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    (void)puts("all VM tests passed");
    return 0;
}
