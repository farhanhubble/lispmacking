/* SPDX-License-Identifier: MPL-2.0 */

#include "vm/internal.h"

static bool exactly_no_arguments(const Vm *vm, const VmObject *arguments)
{
    return arguments == vm->nil;
}

static bool exactly_one_argument(const Vm *vm, const VmObject *arguments)
{
    return arguments->type == VM_CONS &&
           arguments->as.cons.cdr == vm->nil;
}

static bool exactly_two_arguments(const Vm *vm, const VmObject *arguments)
{
    return arguments->type == VM_CONS &&
           arguments->as.cons.cdr->type == VM_CONS &&
           arguments->as.cons.cdr->as.cons.cdr == vm->nil;
}

static void set_machine_error(Vm *vm, VmMachineResult result)
{
    switch (result) {
    case VM_MACHINE_OK:
        break;
    case VM_MACHINE_UNAVAILABLE:
        vm_set_error(vm, "privileged operation unavailable on this platform");
        break;
    case VM_MACHINE_INVALID_ADDRESS:
        vm_set_error(vm, "address is outside allocated bootstrap pages");
        break;
    case VM_MACHINE_MISALIGNED:
        vm_set_error(vm, "address does not satisfy access alignment");
        break;
    case VM_MACHINE_OUT_OF_MEMORY:
        vm_set_error(vm, "no bootstrap pages remain");
        break;
    case VM_MACHINE_INVALID_OPERATION:
        vm_set_error(vm, "privileged operation is not allowlisted");
        break;
    case VM_MACHINE_NOT_EXECUTABLE:
        vm_set_error(vm, "address is not in an executable bootstrap page");
        break;
    }
}

static bool integer_argument(Vm *vm, VmObject *object, uint64_t *value,
                             const char *message)
{
    if (object->type != VM_INTEGER) {
        vm_set_error(vm, message);
        return false;
    }
    *value = (uint64_t)object->as.integer;
    return true;
}

static VmObject *primitive_page_allocate(Vm *vm, VmObject *arguments)
{
    uint64_t address;
    VmMachineResult result;

    if (!exactly_no_arguments(vm, arguments)) {
        vm_set_error(vm, "bootstrap:page-allocate expects no arguments");
        return NULL;
    }
    if (vm->platform.page_allocate == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.page_allocate(vm->platform.context, &address);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    if (address > (uint64_t)INT64_MAX) {
        vm_set_error(vm, "page address cannot be represented as an integer");
        return NULL;
    }
    return vm_integer(vm, (int64_t)address);
}

static VmObject *primitive_code_page_allocate(Vm *vm, VmObject *arguments)
{
    uint64_t address;
    VmMachineResult result;

    if (!exactly_no_arguments(vm, arguments)) {
        vm_set_error(vm, "bootstrap:code-page-allocate expects no arguments");
        return NULL;
    }
    if (vm->platform.code_page_allocate == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.code_page_allocate(vm->platform.context, &address);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    if (address > (uint64_t)INT64_MAX) {
        vm_set_error(vm, "code page address cannot be represented as an integer");
        return NULL;
    }
    return vm_integer(vm, (int64_t)address);
}

static VmObject *memory_read(Vm *vm, VmObject *arguments, size_t width,
                             const char *message)
{
    uint64_t address;
    uint64_t value;
    VmMachineResult result;

    if (!exactly_one_argument(vm, arguments) ||
        !integer_argument(vm, arguments->as.cons.car, &address, message)) {
        if (vm_error(vm)[0] == '\0') {
            vm_set_error(vm, message);
        }
        return NULL;
    }
    if (vm->platform.memory_read == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.memory_read(vm->platform.context, address, width,
                                      &value);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    if (value > (uint64_t)INT64_MAX) {
        vm_set_error(vm, "loaded value cannot be represented as an integer");
        return NULL;
    }
    return vm_integer(vm, (int64_t)value);
}

static VmObject *memory_write(Vm *vm, VmObject *arguments, size_t width,
                              const char *message)
{
    VmObject *value_object;
    uint64_t address;
    uint64_t value;
    uint64_t maximum;
    VmMachineResult result;

    if (!exactly_two_arguments(vm, arguments)) {
        vm_set_error(vm, message);
        return NULL;
    }
    value_object = arguments->as.cons.cdr->as.cons.car;
    if (!integer_argument(vm, arguments->as.cons.car, &address, message) ||
        !integer_argument(vm, value_object, &value, message)) {
        return NULL;
    }
    if (value_object->as.integer < 0) {
        vm_set_error(vm, "stored value must be non-negative");
        return NULL;
    }
    maximum = width == 8 ? UINT64_MAX : (((uint64_t)1 << (width * 8)) - 1);
    if (value > maximum) {
        vm_set_error(vm, "stored value does not fit the requested width");
        return NULL;
    }
    if (vm->platform.memory_write == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.memory_write(vm->platform.context, address, width,
                                       value);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    return value_object;
}

#define DEFINE_MEMORY_READ(function_name, width, message)                     \
    static VmObject *function_name(Vm *vm, VmObject *arguments)               \
    {                                                                          \
        return memory_read(vm, arguments, width, message);                     \
    }

#define DEFINE_MEMORY_WRITE(function_name, width, message)                    \
    static VmObject *function_name(Vm *vm, VmObject *arguments)               \
    {                                                                          \
        return memory_write(vm, arguments, width, message);                    \
    }

DEFINE_MEMORY_READ(primitive_load_u8, 1,
                   "bootstrap:load-u8 expects one integer address")
DEFINE_MEMORY_READ(primitive_load_u16, 2,
                   "bootstrap:load-u16 expects one integer address")
DEFINE_MEMORY_READ(primitive_load_u32, 4,
                   "bootstrap:load-u32 expects one integer address")
DEFINE_MEMORY_READ(primitive_load_u64, 8,
                   "bootstrap:load-u64 expects one integer address")
DEFINE_MEMORY_WRITE(primitive_store_u8, 1,
                    "bootstrap:store-u8 expects integer address and value")
DEFINE_MEMORY_WRITE(primitive_store_u16, 2,
                    "bootstrap:store-u16 expects integer address and value")
DEFINE_MEMORY_WRITE(primitive_store_u32, 4,
                    "bootstrap:store-u32 expects integer address and value")
DEFINE_MEMORY_WRITE(primitive_store_u64, 8,
                    "bootstrap:store-u64 expects integer address and value")

static bool csr_argument(Vm *vm, VmObject *object, VmCsr *csr)
{
    const char *name;

    if (object->type != VM_SYMBOL) {
        vm_set_error(vm, "CSR name must be a quoted symbol");
        return false;
    }
    name = object->as.symbol.name;
    if (name[0] == 'm' && name[1] == 's' && name[2] == 't' &&
        name[3] == 'a' && name[4] == 't' && name[5] == 'u' &&
        name[6] == 's' && name[7] == '\0') {
        *csr = VM_CSR_MSTATUS;
    } else if (name[0] == 'm' && name[1] == 'i' && name[2] == 'e' &&
               name[3] == '\0') {
        *csr = VM_CSR_MIE;
    } else if (name[0] == 'm' && name[1] == 't' && name[2] == 'v' &&
               name[3] == 'e' && name[4] == 'c' && name[5] == '\0') {
        *csr = VM_CSR_MTVEC;
    } else if (name[0] == 'm' && name[1] == 's' && name[2] == 'c' &&
               name[3] == 'r' && name[4] == 'a' && name[5] == 't' &&
               name[6] == 'c' && name[7] == 'h' && name[8] == '\0') {
        *csr = VM_CSR_MSCRATCH;
    } else {
        vm_set_error(vm, "CSR is not allowlisted");
        return false;
    }
    return true;
}

static VmObject *primitive_csr_read(Vm *vm, VmObject *arguments)
{
    VmCsr csr;
    uint64_t value;
    VmMachineResult result;

    if (!exactly_one_argument(vm, arguments) ||
        !csr_argument(vm, arguments->as.cons.car, &csr)) {
        if (vm_error(vm)[0] == '\0') {
            vm_set_error(vm, "bootstrap:csr-read expects one quoted CSR name");
        }
        return NULL;
    }
    if (vm->platform.csr_read == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.csr_read(vm->platform.context, csr, &value);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    if (value > (uint64_t)INT64_MAX) {
        vm_set_error(vm, "CSR value cannot be represented as an integer");
        return NULL;
    }
    return vm_integer(vm, (int64_t)value);
}

static VmObject *primitive_csr_write(Vm *vm, VmObject *arguments)
{
    VmCsr csr;
    VmObject *value_object;
    uint64_t value;
    VmMachineResult result;

    if (!exactly_two_arguments(vm, arguments)) {
        vm_set_error(vm,
                     "bootstrap:csr-write expects quoted CSR name and integer");
        return NULL;
    }
    value_object = arguments->as.cons.cdr->as.cons.car;
    if (!csr_argument(vm, arguments->as.cons.car, &csr) ||
        !integer_argument(vm, value_object, &value,
                          "bootstrap:csr-write expects quoted CSR name and integer")) {
        return NULL;
    }
    if (value_object->as.integer < 0) {
        vm_set_error(vm, "CSR value must be non-negative");
        return NULL;
    }
    if (vm->platform.csr_write == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.csr_write(vm->platform.context, csr, value);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    return value_object;
}

static VmObject *primitive_interrupts_disable(Vm *vm, VmObject *arguments)
{
    uint64_t previous;
    VmMachineResult result;

    if (!exactly_no_arguments(vm, arguments)) {
        vm_set_error(vm, "bootstrap:interrupts-disable expects no arguments");
        return NULL;
    }
    if (vm->platform.interrupts_disable == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.interrupts_disable(vm->platform.context, &previous);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    return vm_integer(vm, (int64_t)previous);
}

static VmObject *primitive_interrupts_restore(Vm *vm, VmObject *arguments)
{
    uint64_t previous;
    VmMachineResult result;

    if (!exactly_one_argument(vm, arguments) ||
        !integer_argument(vm, arguments->as.cons.car, &previous,
                          "bootstrap:interrupts-restore expects a 0 or 1 token")) {
        if (vm_error(vm)[0] == '\0') {
            vm_set_error(vm,
                         "bootstrap:interrupts-restore expects a 0 or 1 token");
        }
        return NULL;
    }
    if (previous > 1) {
        vm_set_error(vm, "interrupt restore token must be 0 or 1");
        return NULL;
    }
    if (vm->platform.interrupts_restore == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.interrupts_restore(vm->platform.context, previous);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    return vm_integer(vm, (int64_t)previous);
}

static VmObject *primitive_call_native(Vm *vm, VmObject *arguments)
{
    VmObject *argument_object;
    uint64_t address;
    uint64_t argument;
    uint64_t result_value;
    int64_t signed_result;
    VmMachineResult result;

    if (!exactly_two_arguments(vm, arguments)) {
        vm_set_error(vm,
                     "bootstrap:call-native expects integer address and argument");
        return NULL;
    }
    argument_object = arguments->as.cons.cdr->as.cons.car;
    if (!integer_argument(vm, arguments->as.cons.car, &address,
                          "bootstrap:call-native expects integer address and argument") ||
        !integer_argument(vm, argument_object, &argument,
                          "bootstrap:call-native expects integer address and argument")) {
        return NULL;
    }
    if (arguments->as.cons.car->as.integer < 0) {
        vm_set_error(vm, "native entry address must be non-negative");
        return NULL;
    }
    if (vm->platform.native_call == NULL) {
        set_machine_error(vm, VM_MACHINE_UNAVAILABLE);
        return NULL;
    }
    result = vm->platform.native_call(vm->platform.context, address, argument,
                                      &result_value);
    if (result != VM_MACHINE_OK) {
        set_machine_error(vm, result);
        return NULL;
    }
    if (result_value <= (uint64_t)INT64_MAX) {
        signed_result = (int64_t)result_value;
    } else {
        signed_result = -1 - (int64_t)(UINT64_MAX - result_value);
    }
    return vm_integer(vm, signed_result);
}

bool vm_install_machine_primitives(Vm *vm)
{
    return vm_bind_primitive(vm, "bootstrap:page-allocate",
                             primitive_page_allocate) &&
           vm_bind_primitive(vm, "bootstrap:code-page-allocate",
                             primitive_code_page_allocate) &&
           vm_bind_primitive(vm, "bootstrap:load-u8", primitive_load_u8) &&
           vm_bind_primitive(vm, "bootstrap:load-u16", primitive_load_u16) &&
           vm_bind_primitive(vm, "bootstrap:load-u32", primitive_load_u32) &&
           vm_bind_primitive(vm, "bootstrap:load-u64", primitive_load_u64) &&
           vm_bind_primitive(vm, "bootstrap:store-u8", primitive_store_u8) &&
           vm_bind_primitive(vm, "bootstrap:store-u16", primitive_store_u16) &&
           vm_bind_primitive(vm, "bootstrap:store-u32", primitive_store_u32) &&
           vm_bind_primitive(vm, "bootstrap:store-u64", primitive_store_u64) &&
           vm_bind_primitive(vm, "bootstrap:csr-read", primitive_csr_read) &&
           vm_bind_primitive(vm, "bootstrap:csr-write", primitive_csr_write) &&
           vm_bind_primitive(vm, "bootstrap:interrupts-disable",
                             primitive_interrupts_disable) &&
           vm_bind_primitive(vm, "bootstrap:interrupts-restore",
                             primitive_interrupts_restore) &&
           vm_bind_primitive(vm, "bootstrap:call-native",
                             primitive_call_native);
}
