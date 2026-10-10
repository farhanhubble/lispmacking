/* SPDX-License-Identifier: MPL-2.0 */

#include "vm/internal.h"

static bool string_equals_span(const char *string, const char *span,
                               size_t length)
{
    size_t index;

    for (index = 0; index < length; ++index) {
        if (string[index] == '\0' || string[index] != span[index]) {
            return false;
        }
    }
    return string[length] == '\0';
}

void vm_clear_error(Vm *vm)
{
    vm->error[0] = '\0';
}

void vm_set_error(Vm *vm, const char *message)
{
    size_t index = 0;

    if (vm->error[0] != '\0') {
        return;
    }
    while (message[index] != '\0' && index + 1 < sizeof(vm->error)) {
        vm->error[index] = message[index];
        ++index;
    }
    vm->error[index] = '\0';
}

const char *vm_error(const Vm *vm)
{
    return vm->error;
}

VmObject *vm_allocate_object(Vm *vm, VmType type)
{
    VmObject *object = vm->platform.allocate(vm->platform.context,
                                             sizeof(*object));
    if (object == NULL) {
        vm_set_error(vm, "out of memory");
        return NULL;
    }
    object->type = type;
    object->allocation_next = vm->allocations;
    vm->allocations = object;
    return object;
}

VmObject *vm_intern_span(Vm *vm, const char *name, size_t length)
{
    VmObject *symbol;
    char *copy;
    size_t index;

    if (length == 3 && name[0] == 'n' && name[1] == 'i' && name[2] == 'l') {
        return vm->nil;
    }
    for (symbol = vm->interned_symbols; symbol != NULL;
         symbol = symbol->as.symbol.intern_next) {
        if (string_equals_span(symbol->as.symbol.name, name, length)) {
            return symbol;
        }
    }

    copy = vm->platform.allocate(vm->platform.context, length + 1);
    if (copy == NULL) {
        vm_set_error(vm, "out of memory");
        return NULL;
    }
    for (index = 0; index < length; ++index) {
        copy[index] = name[index];
    }
    copy[length] = '\0';

    symbol = vm_allocate_object(vm, VM_SYMBOL);
    if (symbol == NULL) {
        vm->platform.deallocate(vm->platform.context, copy);
        return NULL;
    }
    symbol->as.symbol.name = copy;
    symbol->as.symbol.intern_next = vm->interned_symbols;
    vm->interned_symbols = symbol;
    return symbol;
}

VmObject *vm_intern(Vm *vm, const char *name)
{
    size_t length = 0;

    while (name[length] != '\0') {
        ++length;
    }
    return vm_intern_span(vm, name, length);
}

VmObject *vm_cons(Vm *vm, VmObject *car, VmObject *cdr)
{
    VmObject *object;

    if (car == NULL || cdr == NULL) {
        vm_set_error(vm, "cannot construct a cons from a failed value");
        return NULL;
    }
    object = vm_allocate_object(vm, VM_CONS);
    if (object != NULL) {
        object->as.cons.car = car;
        object->as.cons.cdr = cdr;
    }
    return object;
}

VmObject *vm_make_primitive(Vm *vm, const char *name, VmPrimitive function)
{
    VmObject *object = vm_allocate_object(vm, VM_PRIMITIVE);

    if (object != NULL) {
        object->as.primitive.name = name;
        object->as.primitive.function = function;
    }
    return object;
}

VmObject *vm_make_lambda(Vm *vm, VmObject *parameters, VmObject *body,
                         VmObject *environment)
{
    VmObject *object = vm_allocate_object(vm, VM_LAMBDA);

    if (object != NULL) {
        object->as.lambda.parameters = parameters;
        object->as.lambda.body = body;
        object->as.lambda.environment = environment;
    }
    return object;
}

VmObject *vm_environment_bind(Vm *vm, VmObject *environment,
                              VmObject *symbol, VmObject *value)
{
    VmObject *binding = vm_cons(vm, symbol, value);

    if (binding == NULL) {
        return NULL;
    }
    return vm_cons(vm, binding, environment);
}

VmObject *vm_environment_lookup(Vm *vm, VmObject *environment,
                                VmObject *symbol)
{
    while (environment != vm->nil) {
        VmObject *binding;

        if (environment->type != VM_CONS) {
            vm_set_error(vm, "invalid environment");
            return NULL;
        }
        binding = environment->as.cons.car;
        if (binding->type != VM_CONS) {
            vm_set_error(vm, "invalid environment binding");
            return NULL;
        }
        if (binding->as.cons.car == symbol) {
            return binding->as.cons.cdr;
        }
        environment = environment->as.cons.cdr;
    }
    vm_set_error(vm, "unbound symbol");
    return NULL;
}

bool vm_init(Vm *vm, VmPlatform platform)
{
    VmObject *true_binding;

    vm->platform = platform;
    vm->allocations = NULL;
    vm->interned_symbols = NULL;
    vm->nil = NULL;
    vm->true_value = NULL;
    vm->quote_symbol = NULL;
    vm->cond_symbol = NULL;
    vm->lambda_symbol = NULL;
    vm->global_environment = NULL;
    vm_clear_error(vm);

    if (platform.allocate == NULL || platform.deallocate == NULL) {
        vm_set_error(vm, "platform must provide allocation functions");
        return false;
    }

    vm->nil = vm_allocate_object(vm, VM_NIL);
    if (vm->nil == NULL) {
        return false;
    }
    vm->global_environment = vm->nil;
    vm->true_value = vm_intern(vm, "t");
    vm->quote_symbol = vm_intern(vm, "quote");
    vm->cond_symbol = vm_intern(vm, "cond");
    vm->lambda_symbol = vm_intern(vm, "lambda");
    if (vm->true_value == NULL || vm->quote_symbol == NULL ||
        vm->cond_symbol == NULL || vm->lambda_symbol == NULL) {
        return false;
    }

    true_binding = vm_environment_bind(vm, vm->global_environment,
                                       vm->true_value, vm->true_value);
    if (true_binding == NULL) {
        return false;
    }
    vm->global_environment = true_binding;
    return vm_install_primitives(vm);
}

void vm_destroy(Vm *vm)
{
    VmObject *object = vm->allocations;

    while (object != NULL) {
        VmObject *next = object->allocation_next;
        if (object->type == VM_SYMBOL) {
            vm->platform.deallocate(vm->platform.context,
                                    object->as.symbol.name);
        }
        vm->platform.deallocate(vm->platform.context, object);
        object = next;
    }
    vm->allocations = NULL;
    vm->interned_symbols = NULL;
    vm->nil = NULL;
}

VmObject *vm_nil(const Vm *vm)
{
    return vm->nil;
}

VmObject *vm_true(const Vm *vm)
{
    return vm->true_value;
}
