/* SPDX-License-Identifier: MPL-2.0 */

#include "vm/internal.h"

typedef struct {
    Vm *vm;
    const char *cursor;
} Reader;

static bool is_space(char character)
{
    return character == ' ' || character == '\t' || character == '\n' ||
           character == '\r';
}

static bool is_delimiter(char character)
{
    return character == '\0' || is_space(character) || character == '(' ||
           character == ')' || character == '\'' || character == ';';
}

static void skip_layout(Reader *reader)
{
    for (;;) {
        while (is_space(*reader->cursor)) {
            ++reader->cursor;
        }
        if (*reader->cursor != ';') {
            return;
        }
        while (*reader->cursor != '\0' && *reader->cursor != '\n') {
            ++reader->cursor;
        }
    }
}

static VmObject *read_expression(Reader *reader);

static VmObject *read_atom(Reader *reader, const char *start, size_t length)
{
    size_t index = 0;
    bool negative = false;
    bool numeric_candidate;
    uint64_t magnitude = 0;
    uint64_t limit = (uint64_t)INT64_MAX;

    numeric_candidate = start[0] >= '0' && start[0] <= '9';
    if (start[index] == '-') {
        negative = true;
        limit += 1;
        ++index;
        numeric_candidate = index < length && start[index] >= '0' &&
                            start[index] <= '9';
    }
    if (index == length) {
        return vm_intern_span(reader->vm, start, length);
    }
    for (; index < length; ++index) {
        unsigned int digit;

        if (start[index] < '0' || start[index] > '9') {
            if (numeric_candidate) {
                vm_set_error(reader->vm, "malformed integer literal");
                return NULL;
            }
            return vm_intern_span(reader->vm, start, length);
        }
        digit = (unsigned int)(start[index] - '0');
        if (magnitude > (limit - digit) / 10) {
            vm_set_error(reader->vm, "integer literal out of range");
            return NULL;
        }
        magnitude = magnitude * 10 + digit;
    }
    if (negative) {
        if (magnitude == (uint64_t)INT64_MAX + 1) {
            return vm_integer(reader->vm, INT64_MIN);
        }
        return vm_integer(reader->vm, -(int64_t)magnitude);
    }
    return vm_integer(reader->vm, (int64_t)magnitude);
}

static VmObject *read_list(Reader *reader)
{
    VmObject *head = reader->vm->nil;
    VmObject *tail = NULL;

    ++reader->cursor;
    skip_layout(reader);
    while (*reader->cursor != ')') {
        VmObject *element;
        VmObject *cell;

        if (*reader->cursor == '\0') {
            vm_set_error(reader->vm, "unterminated list");
            return NULL;
        }
        if (*reader->cursor == '.' &&
            is_delimiter(reader->cursor[1])) {
            VmObject *last;

            if (tail == NULL) {
                vm_set_error(reader->vm, "dot requires a preceding list item");
                return NULL;
            }
            ++reader->cursor;
            skip_layout(reader);
            last = read_expression(reader);
            if (last == NULL) {
                return NULL;
            }
            skip_layout(reader);
            if (*reader->cursor != ')') {
                vm_set_error(reader->vm, "dotted pair must end after its tail");
                return NULL;
            }
            tail->as.cons.cdr = last;
            ++reader->cursor;
            return head;
        }

        element = read_expression(reader);
        if (element == NULL) {
            return NULL;
        }
        cell = vm_cons(reader->vm, element, reader->vm->nil);
        if (cell == NULL) {
            return NULL;
        }
        if (tail == NULL) {
            head = cell;
        } else {
            tail->as.cons.cdr = cell;
        }
        tail = cell;
        skip_layout(reader);
    }
    ++reader->cursor;
    return head;
}

static VmObject *read_expression(Reader *reader)
{
    const char *start;

    skip_layout(reader);
    if (*reader->cursor == '(') {
        return read_list(reader);
    }
    if (*reader->cursor == '\'') {
        VmObject *quoted;
        VmObject *tail;

        ++reader->cursor;
        quoted = read_expression(reader);
        if (quoted == NULL) {
            return NULL;
        }
        tail = vm_cons(reader->vm, quoted, reader->vm->nil);
        if (tail == NULL) {
            return NULL;
        }
        return vm_cons(reader->vm, reader->vm->quote_symbol, tail);
    }
    if (*reader->cursor == ')') {
        vm_set_error(reader->vm, "unexpected closing parenthesis");
        return NULL;
    }
    if (*reader->cursor == '\0') {
        return NULL;
    }

    start = reader->cursor;
    while (!is_delimiter(*reader->cursor)) {
        ++reader->cursor;
    }
    if (reader->cursor == start) {
        vm_set_error(reader->vm, "expected a symbol");
        return NULL;
    }
    return read_atom(reader, start, (size_t)(reader->cursor - start));
}

VmObject *vm_read(Vm *vm, const char *source, const char **end)
{
    Reader reader;
    VmObject *result;

    reader.vm = vm;
    reader.cursor = source;
    vm_clear_error(vm);
    skip_layout(&reader);
    if (*reader.cursor == '\0') {
        if (end != NULL) {
            *end = reader.cursor;
        }
        return NULL;
    }
    result = read_expression(&reader);
    skip_layout(&reader);
    if (end != NULL) {
        *end = reader.cursor;
    }
    return result;
}
