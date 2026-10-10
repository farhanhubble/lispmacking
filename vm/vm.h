/* SPDX-License-Identifier: MPL-2.0 */

#ifndef LISP_MACHINE_VM_H
#define LISP_MACHINE_VM_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Vm Vm;
typedef struct VmObject VmObject;

typedef void *(*VmAllocate)(void *context, size_t size);
typedef void (*VmDeallocate)(void *context, void *pointer);
typedef void (*VmWrite)(void *context, const char *bytes, size_t length);

typedef struct {
    void *context;
    VmAllocate allocate;
    VmDeallocate deallocate;
} VmPlatform;

typedef enum {
    VM_NIL,
    VM_SYMBOL,
    VM_CONS,
    VM_PRIMITIVE,
    VM_LAMBDA,
} VmType;

typedef VmObject *(*VmPrimitive)(Vm *vm, VmObject *arguments);

/*
 * Every Lisp value is a stable pointer to one tagged object. Objects live until
 * vm_destroy; the initial VM deliberately has no collector. VM_NIL is a singleton,
 * symbols are interned, and cons fields always point to objects in the same VM.
 */
struct VmObject {
    VmType type;
    VmObject *allocation_next;
    union {
        struct {
            char *name;
            VmObject *intern_next;
        } symbol;
        struct {
            VmObject *car;
            VmObject *cdr;
        } cons;
        struct {
            const char *name;
            VmPrimitive function;
        } primitive;
        struct {
            VmObject *parameters;
            VmObject *body;
            VmObject *environment;
        } lambda;
    } as;
};

struct Vm {
    VmPlatform platform;
    VmObject *allocations;
    VmObject *interned_symbols;
    VmObject *nil;
    VmObject *true_value;
    VmObject *quote_symbol;
    VmObject *cond_symbol;
    VmObject *lambda_symbol;
    VmObject *global_environment;
    char error[160];
};

bool vm_init(Vm *vm, VmPlatform platform);
void vm_destroy(Vm *vm);

VmObject *vm_nil(const Vm *vm);
VmObject *vm_true(const Vm *vm);
VmObject *vm_intern(Vm *vm, const char *name);
VmObject *vm_cons(Vm *vm, VmObject *car, VmObject *cdr);

VmObject *vm_read(Vm *vm, const char *source, const char **end);
VmObject *vm_eval(Vm *vm, VmObject *expression, VmObject *environment);
VmObject *vm_eval_source(Vm *vm, const char *source);
void vm_print(const VmObject *object, VmWrite write, void *context);

const char *vm_error(const Vm *vm);
void vm_clear_error(Vm *vm);

#endif
