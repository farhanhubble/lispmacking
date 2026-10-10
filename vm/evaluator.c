#include "vm/internal.h"

static bool is_proper_list(const Vm *vm, const VmObject *list)
{
    while (list != vm->nil) {
        if (list->type != VM_CONS) {
            return false;
        }
        list = list->as.cons.cdr;
    }
    return true;
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

static VmObject *primitive_atom(Vm *vm, VmObject *arguments)
{
    if (!exactly_one_argument(vm, arguments)) {
        vm_set_error(vm, "atom expects one argument");
        return NULL;
    }
    return arguments->as.cons.car->type == VM_CONS ? vm->nil : vm->true_value;
}

static VmObject *primitive_eq(Vm *vm, VmObject *arguments)
{
    VmObject *first;
    VmObject *second;

    if (!exactly_two_arguments(vm, arguments)) {
        vm_set_error(vm, "eq expects two arguments");
        return NULL;
    }
    first = arguments->as.cons.car;
    second = arguments->as.cons.cdr->as.cons.car;
    return first == second ? vm->true_value : vm->nil;
}

static VmObject *primitive_car(Vm *vm, VmObject *arguments)
{
    VmObject *pair;

    if (!exactly_one_argument(vm, arguments)) {
        vm_set_error(vm, "car expects one argument");
        return NULL;
    }
    pair = arguments->as.cons.car;
    if (pair->type != VM_CONS) {
        vm_set_error(vm, "car expects a cons");
        return NULL;
    }
    return pair->as.cons.car;
}

static VmObject *primitive_cdr(Vm *vm, VmObject *arguments)
{
    VmObject *pair;

    if (!exactly_one_argument(vm, arguments)) {
        vm_set_error(vm, "cdr expects one argument");
        return NULL;
    }
    pair = arguments->as.cons.car;
    if (pair->type != VM_CONS) {
        vm_set_error(vm, "cdr expects a cons");
        return NULL;
    }
    return pair->as.cons.cdr;
}

static VmObject *primitive_cons(Vm *vm, VmObject *arguments)
{
    if (!exactly_two_arguments(vm, arguments)) {
        vm_set_error(vm, "cons expects two arguments");
        return NULL;
    }
    return vm_cons(vm, arguments->as.cons.car,
                   arguments->as.cons.cdr->as.cons.car);
}

static bool bind_primitive(Vm *vm, const char *name, VmPrimitive function)
{
    VmObject *symbol = vm_intern(vm, name);
    VmObject *primitive = vm_make_primitive(vm, name, function);
    VmObject *environment;

    if (symbol == NULL || primitive == NULL) {
        return false;
    }
    environment = vm_environment_bind(vm, vm->global_environment,
                                      symbol, primitive);
    if (environment == NULL) {
        return false;
    }
    vm->global_environment = environment;
    return true;
}

bool vm_install_primitives(Vm *vm)
{
    return bind_primitive(vm, "atom", primitive_atom) &&
           bind_primitive(vm, "eq", primitive_eq) &&
           bind_primitive(vm, "car", primitive_car) &&
           bind_primitive(vm, "cdr", primitive_cdr) &&
           bind_primitive(vm, "cons", primitive_cons);
}

static VmObject *eval_arguments(Vm *vm, VmObject *expressions,
                                VmObject *environment)
{
    VmObject *head = vm->nil;
    VmObject *tail = NULL;

    while (expressions != vm->nil) {
        VmObject *value;
        VmObject *cell;

        if (expressions->type != VM_CONS) {
            vm_set_error(vm, "application arguments must form a proper list");
            return NULL;
        }
        value = vm_eval(vm, expressions->as.cons.car, environment);
        if (value == NULL) {
            return NULL;
        }
        cell = vm_cons(vm, value, vm->nil);
        if (cell == NULL) {
            return NULL;
        }
        if (tail == NULL) {
            head = cell;
        } else {
            tail->as.cons.cdr = cell;
        }
        tail = cell;
        expressions = expressions->as.cons.cdr;
    }
    return head;
}

static VmObject *extend_environment(Vm *vm, VmObject *parameters,
                                    VmObject *arguments,
                                    VmObject *environment)
{
    while (parameters != vm->nil && arguments != vm->nil) {
        VmObject *next;
        VmObject *parameter;

        if (parameters->type != VM_CONS || arguments->type != VM_CONS) {
            vm_set_error(vm, "lambda parameters and arguments must be lists");
            return NULL;
        }
        parameter = parameters->as.cons.car;
        if (parameter->type != VM_SYMBOL) {
            vm_set_error(vm, "lambda parameter must be a symbol");
            return NULL;
        }
        next = vm_environment_bind(vm, environment, parameter,
                                   arguments->as.cons.car);
        if (next == NULL) {
            return NULL;
        }
        environment = next;
        parameters = parameters->as.cons.cdr;
        arguments = arguments->as.cons.cdr;
    }
    if (parameters != vm->nil || arguments != vm->nil) {
        vm_set_error(vm, "lambda argument count mismatch");
        return NULL;
    }
    return environment;
}

static VmObject *eval_cond(Vm *vm, VmObject *clauses,
                           VmObject *environment)
{
    while (clauses != vm->nil) {
        VmObject *clause;
        VmObject *predicate;

        if (clauses->type != VM_CONS) {
            vm_set_error(vm, "cond clauses must form a proper list");
            return NULL;
        }
        clause = clauses->as.cons.car;
        if (!exactly_two_arguments(vm, clause)) {
            vm_set_error(vm, "each cond clause requires a test and result");
            return NULL;
        }
        predicate = vm_eval(vm, clause->as.cons.car, environment);
        if (predicate == NULL) {
            return NULL;
        }
        if (predicate != vm->nil) {
            return vm_eval(vm, clause->as.cons.cdr->as.cons.car, environment);
        }
        clauses = clauses->as.cons.cdr;
    }
    return vm->nil;
}

static VmObject *eval_lambda(Vm *vm, VmObject *arguments,
                             VmObject *environment)
{
    VmObject *parameters;
    VmObject *cursor;

    if (!exactly_two_arguments(vm, arguments)) {
        vm_set_error(vm, "lambda requires a parameter list and body");
        return NULL;
    }
    parameters = arguments->as.cons.car;
    if (!is_proper_list(vm, parameters)) {
        vm_set_error(vm, "lambda parameters must form a proper list");
        return NULL;
    }
    for (cursor = parameters; cursor != vm->nil;
         cursor = cursor->as.cons.cdr) {
        if (cursor->as.cons.car->type != VM_SYMBOL) {
            vm_set_error(vm, "lambda parameter must be a symbol");
            return NULL;
        }
    }
    return vm_make_lambda(vm, parameters, arguments->as.cons.cdr->as.cons.car,
                          environment);
}

static VmObject *apply(Vm *vm, VmObject *function, VmObject *arguments)
{
    if (function->type == VM_PRIMITIVE) {
        return function->as.primitive.function(vm, arguments);
    }
    if (function->type == VM_LAMBDA) {
        VmObject *environment = extend_environment(
            vm, function->as.lambda.parameters, arguments,
            function->as.lambda.environment);
        if (environment == NULL) {
            return NULL;
        }
        return vm_eval(vm, function->as.lambda.body, environment);
    }
    vm_set_error(vm, "attempted to call a non-function");
    return NULL;
}

VmObject *vm_eval(Vm *vm, VmObject *expression, VmObject *environment)
{
    VmObject *operator_expression;
    VmObject *arguments;
    VmObject *function;
    VmObject *values;

    if (expression == NULL || environment == NULL) {
        vm_set_error(vm, "cannot evaluate a failed value");
        return NULL;
    }
    if (expression->type == VM_NIL || expression->type == VM_PRIMITIVE ||
        expression->type == VM_LAMBDA) {
        return expression;
    }
    if (expression->type == VM_SYMBOL) {
        return vm_environment_lookup(vm, environment, expression);
    }
    if (expression->type != VM_CONS) {
        vm_set_error(vm, "unknown expression type");
        return NULL;
    }

    operator_expression = expression->as.cons.car;
    arguments = expression->as.cons.cdr;
    if (operator_expression == vm->quote_symbol) {
        if (!exactly_one_argument(vm, arguments)) {
            vm_set_error(vm, "quote expects one argument");
            return NULL;
        }
        return arguments->as.cons.car;
    }
    if (operator_expression == vm->cond_symbol) {
        return eval_cond(vm, arguments, environment);
    }
    if (operator_expression == vm->lambda_symbol) {
        return eval_lambda(vm, arguments, environment);
    }

    function = vm_eval(vm, operator_expression, environment);
    if (function == NULL) {
        return NULL;
    }
    values = eval_arguments(vm, arguments, environment);
    if (values == NULL) {
        return NULL;
    }
    return apply(vm, function, values);
}

VmObject *vm_eval_source(Vm *vm, const char *source)
{
    const char *end;
    VmObject *expression = vm_read(vm, source, &end);

    if (expression == NULL) {
        if (vm_error(vm)[0] == '\0') {
            vm_set_error(vm, "expected an expression");
        }
        return NULL;
    }
    if (*end != '\0') {
        vm_set_error(vm, "unexpected input after expression");
        return NULL;
    }
    return vm_eval(vm, expression, vm->global_environment);
}
