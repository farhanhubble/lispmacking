/* SPDX-License-Identifier: MPL-2.0 */

#ifndef LISP_MACHINE_VM_INTERNAL_H
#define LISP_MACHINE_VM_INTERNAL_H

#include "vm/vm.h"

VmObject *vm_allocate_object(Vm *vm, VmType type);
VmObject *vm_intern_span(Vm *vm, const char *name, size_t length);
VmObject *vm_make_primitive(Vm *vm, const char *name, VmPrimitive function);
VmObject *vm_make_lambda(Vm *vm, VmObject *parameters, VmObject *body,
                         VmObject *environment);
VmObject *vm_environment_bind(Vm *vm, VmObject *environment,
                              VmObject *symbol, VmObject *value);
VmObject *vm_environment_lookup(Vm *vm, VmObject *environment,
                                VmObject *symbol);
bool vm_install_primitives(Vm *vm);
bool vm_install_machine_primitives(Vm *vm);
bool vm_bind_primitive(Vm *vm, const char *name, VmPrimitive function);
void vm_set_error(Vm *vm, const char *message);

#endif
