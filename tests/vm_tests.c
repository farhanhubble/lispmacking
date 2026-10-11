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

static void expect_error_contains(Vm *vm, const char *source,
                                  const char *expected)
{
    VmObject *result = vm_eval_source(vm, source);

    if (result != NULL || strstr(vm_error(vm), expected) == NULL) {
        (void)fprintf(stderr, "FAIL: %s -> error: %s (expected %s)\n",
                      source, vm_error(vm), expected);
        ++failures;
    }
}

static void expect_integer_round_trip(Vm *vm, const char *source,
                                      int64_t expected)
{
    VmObject *first = vm_read(vm, source, NULL);
    Buffer buffer = {{0}, 0};
    VmObject *second;

    if (first == NULL) {
        (void)fprintf(stderr, "FAIL: cannot read %s: %s\n", source,
                      vm_error(vm));
        ++failures;
        return;
    }
    vm_print(first, buffer_write, &buffer);
    second = vm_read(vm, buffer.bytes, NULL);
    if (first->type != VM_INTEGER || second == NULL ||
        second->type != VM_INTEGER || first->as.integer != expected ||
        second->as.integer != expected) {
        (void)fprintf(stderr, "FAIL: integer round trip for %s via %s\n",
                      source, buffer.bytes);
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

    expect_eval(&vm, "0", "0");
    expect_eval(&vm, "-0", "0");
    expect_eval(&vm, "9223372036854775807", "9223372036854775807");
    expect_eval(&vm, "-9223372036854775808", "-9223372036854775808");
    expect_integer_round_trip(&vm, "9223372036854775807", INT64_MAX);
    expect_integer_round_trip(&vm, "-9223372036854775808", INT64_MIN);
    expect_eval(&vm, "(+ 40 2)", "42");
    expect_eval(&vm, "(- -40 2)", "-42");
    expect_eval(&vm, "(* -6 -7)", "42");
    expect_eval(&vm, "(* -9223372036854775808 1)",
                "-9223372036854775808");
    expect_eval(&vm, "(= 42 42)", "t");
    expect_eval(&vm, "(eq 1 1)", "nil");
    expect_eval(&vm, "(< -1 0)", "t");
    expect_eval(&vm, "(<= 1 1)", "t");
    expect_eval(&vm, "(> 2 1)", "t");
    expect_eval(&vm, "(>= 1 2)", "nil");
    expect_eval(&vm, "((lambda (x) (+ x 1)) 41)", "42");
    expect_eval(&vm, "(quotient 43 2)", "21");
    expect_eval(&vm, "(mod 43 2)", "1");
    expect_eval(&vm, "(integer? 42)", "t");
    expect_eval(&vm, "(integer? 'forty-two)", "nil");
    expect_eval(&vm, "(begin 1 2 3)", "3");
    expect_eval(&vm, "(begin)", "nil");
    expect_eval(&vm, "(define increment (lambda (x) (+ x 1)))",
                "<lambda>");
    expect_eval(&vm, "(increment 41)", "42");
    expect_eval(&vm,
                "(define sum-down (lambda (x) (cond ((= x 0) 0) "
                "(t (+ x (sum-down (- x 1)))))))",
                "<lambda>");
    expect_eval(&vm, "(sum-down 6)", "21");
    expect_eval(&vm,
                "(define captured ((lambda (x) (lambda () x)) 42))",
                "<lambda>");
    expect_eval(&vm, "(captured)", "42");

    expect_error(&vm, "missing");
    expect_error(&vm, "(car 'a)");
    expect_error(&vm, "((lambda (x) x) 'a 'b)");
    expect_error(&vm, "(quote a b)");
    expect_error(&vm, "(a");
    expect_error(&vm, "'a 'b");
    expect_error_contains(&vm, "9223372036854775808", "out of range");
    expect_error_contains(&vm, "-9223372036854775809", "out of range");
    expect_error_contains(&vm, "12oops", "malformed integer");
    expect_error_contains(&vm, "(+ 9223372036854775807 1)", "overflow");
    expect_error_contains(&vm, "(- -9223372036854775808 1)", "overflow");
    expect_error_contains(&vm, "(* 9223372036854775807 2)", "overflow");
    expect_error_contains(&vm, "(* -9223372036854775808 -1)", "overflow");
    expect_error_contains(&vm, "(+ 1 'a)", "expects two integers");
    expect_error_contains(&vm, "(quotient 1 0)", "division by zero");
    expect_error_contains(&vm, "((lambda () (define nested 1)))",
                          "only valid at top level");
    expect_error_contains(&vm, "(error 'intentional-error)",
                          "intentional-error");
    expect_error_contains(&vm, "(bootstrap:page-allocate)", "unavailable");
    expect_error_contains(&vm, "(bootstrap:code-page-allocate)",
                          "unavailable");
    expect_error_contains(&vm, "(bootstrap:load-u8 0)", "unavailable");
    expect_error_contains(&vm, "(bootstrap:store-u8 'address 1)",
                          "expects integer address and value");
    expect_error_contains(&vm, "(bootstrap:csr-read 'cycle)",
                          "not allowlisted");
    expect_error_contains(&vm, "(bootstrap:interrupts-disable)",
                          "unavailable");
    expect_error_contains(&vm, "(bootstrap:interrupts-restore 2)",
                          "must be 0 or 1");
    expect_error_contains(&vm, "(bootstrap:call-native 0 1)",
                          "unavailable");

    vm_destroy(&vm);
    if (failures != 0) {
        (void)fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    (void)puts("all VM tests passed");
    return 0;
}
