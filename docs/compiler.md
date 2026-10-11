# Lisp-written RV64 assembler and compiler

Milestone 5 adds the first self-hosting step above the native C evaluator. The
assembler and compiler are ordinary Lisp source files interpreted by the VM;
they calculate and write RV64 instruction words without C instruction
templates.

```text
Lisp function specifications
          │
          ▼
 lisp/compiler.lisp       expression traversal and instruction selection
          │
          ▼
 lisp/assembler.lisp      R/I/S/B/J instruction encoding
          │
          ▼
 bootstrap:store-u32      bounded writes to an allocated code page
          │
          ▼
 bootstrap:call-native    fence.i, validated entry, RV64 execution
```

## Source representation

`compiler-compile-program` accepts a code-page address and a quoted list of
function specifications:

```lisp
(compiler-compile-program code-page
  '((twice (x) (+ x x))
    (positive (x) (cond ((< x 0) 0)
                        (t (twice x))))))
```

Each function has one name, one parameter, and one body expression. The
restricted body language supports:

- decimal integer literals from `-2048` through `2047`;
- the function parameter as a local value;
- binary `+`, `-`, `*`, and `<` expressions;
- a two-clause `cond` whose second test is `t`;
- calls to compiled functions with one argument;
- `(load-u64 address)` and `(store-u64 address value)`.

Unknown locals and functions, out-of-range literals, oversized functions, and
programs with more than sixteen functions produce recoverable compiler errors.
The compiler is intentionally a small bootstrap compiler, not yet a compiler
for the entire interpreted language.

## Code layout and ABI

One 4 KiB executable bootstrap page holds at most sixteen fixed 256-byte
function slots. Fixed slots make all function addresses available during the
first pass; the second pass emits code and resolves PC-relative calls.

Generated functions use this RV64 calling convention:

| Purpose | Convention |
|---|---|
| Argument | Signed 64-bit integer in `a0` |
| Result | Signed 64-bit integer in `a0` |
| Parameter local | Preserved in `s0` |
| Return address | `ra`, saved in the function frame |
| Expression temporary | 16-byte-aligned stack slot |
| Scratch register | `t0` |

Every function creates a 16-byte frame, saves `ra` and `s0`, and restores both
before `jalr` returns. Nested expression temporaries also reserve 16 bytes, so
the stack remains ABI-aligned across generated calls.

Conditional compilation first emits placeholder branch and jump words. Once
both arms have known offsets, Lisp rewrites those words with the final B- and
J-format displacements. Function calls use `jal` from the current instruction
to the target function's fixed slot.

## Assembler

`lisp/assembler.lisp` implements R-, I-, S-, B-, and J-format encoders using
Lisp integer arithmetic. It currently exposes encoders for `addi`, `add`,
`sub`, `mul`, `slt`, `ld`, `sd`, `beq`, `jal`, and `jalr`. Signed immediate
ranges and instruction alignment are checked before encoding.

Hosted tests compare representative decimal instruction words with encodings
produced independently by the GNU RISC-V assembler. Actual emission is only
available on RV64 because it uses the bounded bootstrap memory interface.

## Execution boundary

`bootstrap:code-page-allocate` marks a page executable within the eight-page
bootstrap pool. `bootstrap:call-native` accepts only a naturally aligned entry
inside an allocated code page, executes `fence.i`, calls it with one signed
integer, and returns its signed result. An ordinary data page is never a valid
native entry.

This distinction prevents accidental jumps to arbitrary RAM, but it is not
W^X isolation: code pages remain writable while this bootstrap compiler is
active, and execution still occurs in machine mode. Milestone 6 must replace
this mechanism with page-table permissions and a user/supervisor boundary.

## Reproducible demonstration

```sh
make test
make test-qemu
```

The hosted test checks reusable definitions, recursion, sequencing, and pure
instruction encodings. The QEMU test streams both Lisp source files into the
serial REPL, compiles four functions, verifies native and evaluated results
match, executes a generated cross-function call and conditional branch, stores
and loads through generated native code, rejects invalid entry points, and
returns cleanly to the REPL.
