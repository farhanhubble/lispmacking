# Machine-oriented Lisp interface

Milestone 4 adds signed 64-bit integers and a deliberately small privileged
bootstrap interface. The ordinary evaluator remains platform-independent;
hardware operations are callbacks supplied by the bare-metal adapter.

## Integers

Integer literals match `-?[0-9]+` and range from `-9223372036854775808` through
`9223372036854775807`. A numeric-looking token that begins with a digit but
does not match that syntax is malformed. The reader rejects malformed and
out-of-range literals, and the printer emits a canonical decimal
representation.

An integer is a separately allocated, tagged VM object containing one
`int64_t`. It remains an atom and cannot be confused with a symbol, cons cell,
or the `nil` singleton. `eq` therefore retains object identity semantics—even
for integers—while `=` is the numeric equality operation.

The ordinary primitives are:

| Primitive | Meaning |
|---|---|
| `+`, `-`, `*` | Checked signed arithmetic on two integers |
| `=`, `<`, `<=`, `>`, `>=` | Signed comparison of two integers |

Arithmetic overflow is a recoverable evaluator error; it never wraps.
`eq` retains its original object-identity semantics, while `=` compares integer
values.

## Privilege boundary

Every operation that reaches machine state begins with `bootstrap:`. These
names are intentionally conspicuous: they are temporary machine-mode tools for
bringing up the compiler and kernel boundary, not operations intended for
ordinary future Lisp worlds. The hosted adapter installs the names so programs
receive a clear `privileged operation unavailable on this platform` error
rather than silently receiving simulated authority.

### Page-backed memory

`(bootstrap:page-allocate)` returns the base address of a zero-filled 4096-byte
page. Eight pages (32 KiB) are reserved at the top of QEMU RAM. Pages are
aligned, non-overlapping, and allocated downward within that region. VM objects
grow upward only as far as the bottom of the reserved pool, so the regions
cannot overlap. A ninth allocation returns a recoverable exhaustion error.

The bounded access primitives are:

```text
(bootstrap:load-u8 address)
(bootstrap:load-u16 address)
(bootstrap:load-u32 address)
(bootstrap:load-u64 address)
(bootstrap:store-u8 address value)
(bootstrap:store-u16 address value)
(bootstrap:store-u32 address value)
(bootstrap:store-u64 address value)
```

An address must fall wholly within pages returned by
`bootstrap:page-allocate` and be naturally aligned for its access width. Store
values are non-negative and must fit the requested width. Loads are unsigned;
a 64-bit loaded value above the Lisp signed-integer maximum is rejected as
unrepresentable. Invalid addresses, alignment violations, and exhausted pages
are recoverable errors.

### CSRs and interrupt state

`(bootstrap:csr-read 'name)` accepts `mstatus`, `mie`, `mtvec`, and `mscratch`.
`(bootstrap:csr-write 'name value)` accepts only `mie`, `mtvec`, and
`mscratch`; whole-register writes to `mstatus` are deliberately prohibited.
All other names are rejected before reaching hardware.

`(bootstrap:interrupts-disable)` atomically clears machine interrupt enable and
returns a `0` or `1` restoration token. `(bootstrap:interrupts-restore token)`
restores that previous state. Nesting is safe when each caller restores its own
token in reverse order.

These operations currently run in RISC-V machine mode. Milestone 6 must replace
their broad bootstrap authority with capabilities and a real user/supervisor
boundary.
