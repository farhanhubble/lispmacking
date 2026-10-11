/* SPDX-License-Identifier: MPL-2.0 */

#ifndef LISP_MACHINE_VM_H
#define LISP_MACHINE_VM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Vm Vm;
typedef struct VmObject VmObject;

typedef void *(*VmAllocate)(void *context, size_t size);
typedef void (*VmDeallocate)(void *context, void *pointer);
typedef void (*VmWrite)(void *context, const char *bytes, size_t length);

typedef enum {
    VM_MACHINE_OK,
    VM_MACHINE_UNAVAILABLE,
    VM_MACHINE_INVALID_ADDRESS,
    VM_MACHINE_MISALIGNED,
    VM_MACHINE_OUT_OF_MEMORY,
    VM_MACHINE_INVALID_OPERATION,
    VM_MACHINE_NOT_EXECUTABLE,
} VmMachineResult;

typedef enum {
    VM_CSR_MSTATUS,
    VM_CSR_MIE,
    VM_CSR_MTVEC,
    VM_CSR_MSCRATCH,
} VmCsr;

typedef VmMachineResult (*VmPageAllocate)(void *context, uint64_t *address);
typedef VmMachineResult (*VmMemoryRead)(void *context, uint64_t address,
                                        size_t width, uint64_t *value);
typedef VmMachineResult (*VmMemoryWrite)(void *context, uint64_t address,
                                         size_t width, uint64_t value);
typedef VmMachineResult (*VmCsrRead)(void *context, VmCsr csr,
                                     uint64_t *value);
typedef VmMachineResult (*VmCsrWrite)(void *context, VmCsr csr,
                                      uint64_t value);
typedef VmMachineResult (*VmInterruptsDisable)(void *context,
                                               uint64_t *previously_enabled);
typedef VmMachineResult (*VmInterruptsRestore)(void *context,
                                               uint64_t previously_enabled);
typedef VmMachineResult (*VmNativeCall)(void *context, uint64_t address,
                                        uint64_t argument, uint64_t *result);

typedef struct {
    void *context;
    VmAllocate allocate;
    VmDeallocate deallocate;
    VmPageAllocate page_allocate;
    VmPageAllocate code_page_allocate;
    VmMemoryRead memory_read;
    VmMemoryWrite memory_write;
    VmCsrRead csr_read;
    VmCsrWrite csr_write;
    VmInterruptsDisable interrupts_disable;
    VmInterruptsRestore interrupts_restore;
    VmNativeCall native_call;
} VmPlatform;

typedef enum {
    VM_NIL,
    VM_SYMBOL,
    VM_CONS,
    VM_PRIMITIVE,
    VM_LAMBDA,
    VM_INTEGER,
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
        int64_t integer;
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
    VmObject *define_symbol;
    VmObject *begin_symbol;
    VmObject *global_environment;
    size_t evaluation_depth;
    char error[160];
};

bool vm_init(Vm *vm, VmPlatform platform);
void vm_destroy(Vm *vm);

VmObject *vm_nil(const Vm *vm);
VmObject *vm_true(const Vm *vm);
VmObject *vm_intern(Vm *vm, const char *name);
VmObject *vm_cons(Vm *vm, VmObject *car, VmObject *cdr);
VmObject *vm_integer(Vm *vm, int64_t value);

VmObject *vm_read(Vm *vm, const char *source, const char **end);
VmObject *vm_eval(Vm *vm, VmObject *expression, VmObject *environment);
VmObject *vm_eval_source(Vm *vm, const char *source);
void vm_print(const VmObject *object, VmWrite write, void *context);

const char *vm_error(const Vm *vm);
void vm_clear_error(Vm *vm);

#endif
