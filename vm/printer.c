/* SPDX-License-Identifier: MPL-2.0 */

#include "vm/vm.h"

static void write_text(VmWrite write, void *context, const char *text)
{
    size_t length = 0;

    while (text[length] != '\0') {
        ++length;
    }
    write(context, text, length);
}

static void print_integer(int64_t value, VmWrite write, void *context)
{
    char digits[21];
    size_t length = 0;
    uint64_t magnitude;

    if (value < 0) {
        write_text(write, context, "-");
        magnitude = (uint64_t)(-(value + 1)) + 1;
    } else {
        magnitude = (uint64_t)value;
    }
    do {
        digits[length++] = (char)('0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude != 0);
    while (length != 0) {
        --length;
        write(context, &digits[length], 1);
    }
}

static void print_object(const VmObject *object, VmWrite write, void *context)
{
    const VmObject *cursor;

    switch (object->type) {
    case VM_NIL:
        write_text(write, context, "nil");
        break;
    case VM_SYMBOL:
        write_text(write, context, object->as.symbol.name);
        break;
    case VM_CONS:
        write_text(write, context, "(");
        cursor = object;
        while (cursor->type == VM_CONS) {
            print_object(cursor->as.cons.car, write, context);
            cursor = cursor->as.cons.cdr;
            if (cursor->type == VM_CONS) {
                write_text(write, context, " ");
            }
        }
        if (cursor->type != VM_NIL) {
            write_text(write, context, " . ");
            print_object(cursor, write, context);
        }
        write_text(write, context, ")");
        break;
    case VM_PRIMITIVE:
        write_text(write, context, "<primitive:");
        write_text(write, context, object->as.primitive.name);
        write_text(write, context, ">");
        break;
    case VM_LAMBDA:
        write_text(write, context, "<lambda>");
        break;
    case VM_INTEGER:
        print_integer(object->as.integer, write, context);
        break;
    }
}

void vm_print(const VmObject *object, VmWrite write, void *context)
{
    if (object != NULL && write != NULL) {
        print_object(object, write, context);
    }
}
