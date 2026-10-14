<!--
title: Bare Metal Enlightenment
subtitle: How one question about SectorLISP became an attempt to grow a Lisp machine on RISC-V
status: substantial draft
hero: ../assets/artwork/lispmacking-series-hero.png
reading_time: 7 minutes
-->

# Bare Metal Enlightenment

The project began with a question I thought I already knew the answer to.

> What’s so cool about SectorLISP?

I typed it into ChatGPT one day, not as someone encountering either half of
the name for the first time. I knew about bare-metal programming. I had
written enough Lisp to understand its central trick. I knew that SectorLISP
could boot directly into a tiny Lisp interpreter.

I knew all the facts. I had not understood the connection.

ChatGPT’s answer began with an almost unreasonable comparison: a complete,
programmable, garbage-collected Lisp could be smaller than an empty modern
word-processing document. The current core of SectorLISP is roughly 436 bytes
of 16-bit x86 machine code, packaged as a 512-byte boot sector. A BIOS can load
it at `0x7c00`, transfer control to it and arrive directly in Lisp.

No operating system is waiting underneath. There is no filesystem, C runtime,
standard library or virtual machine quietly doing the difficult work.

That was not new information to me. This time, however, it landed differently.

## The byte count is the bait

It is natural to look at SectorLISP as an extraordinary piece of code golf.
The [official project](https://github.com/jart/sectorlisp) contains a bootable
i8086 implementation, a portable C reference implementation and a pure Lisp
evaluator. The [436-byte version](https://justine.lol/sectorlisp2/) even
includes a copying garbage collector.

That is an astonishing engineering result. But the number is not the part
that stayed with me.

The profound claim is that a useful language does not have to arrive after the
machine has acquired an operating system, a process model, a runtime loader and
several layers of libraries. A sufficiently small evaluator can be among the
first ideas the machine encounters.

Lisp is unusually well suited to this experiment. Its source language and its
runtime data share the same basic representation. Lists hold programs, but
they also hold the data manipulated by those programs. Once a reader can
construct atoms and cons cells, the evaluator does not need a separate abstract
syntax tree, bytecode file or compiler intermediate representation before it
can begin.

At the centre is a remarkably small vocabulary:

- atoms and cons cells;
- `car`, `cdr` and `cons`;
- quotation;
- tests and conditionals;
- function application.

Even integers are not required for universal computation. They can be built
structurally from lists when convenience and speed are not the objective.

The 436 bytes are therefore evidence, not merely a record. They demonstrate
how little physical machinery is required before symbolic evaluation can
begin.

## The missing connection

I had previously kept two mental models in separate drawers.

Bare-metal programming meant reset vectors, linker scripts, stack pointers,
memory maps and device registers. It was about accepting every obligation an
operating system normally hides.

Lisp meant symbolic expressions, recursive evaluation, closures and the
peculiar pleasure of treating programs as ordinary data.

SectorLISP joined the drawers.

![Bare metal, Lisp and SectorLISP meet at the evaluator](../assets/diagrams/bare-metal-enlightenment.svg)

*The evaluator is the hinge: machine instructions implement a small symbolic
world, and that world can then describe more of itself.*

The native assembly evaluator is small but capable enough to evaluate a Lisp
evaluator written in Lisp—a descendant of McCarthy’s meta-circular evaluator.
The Lisp-written evaluator is not hidden inside the boot sector; it is supplied
as input. That distinction matters. The tiny native core does not contain the
whole future system. It contains enough semantics for the next layer to exist.

Conceptually, the path looks like this:

```text
hardware
  → tiny native Lisp evaluator
      → Lisp evaluator expressed in Lisp
          → the programs built above it
```

I had understood bootstrapping as a compiler technique. SectorLISP made me see
it as an architectural technique. A machine can begin with a small trusted
seed, then use the language brought to life by that seed to construct more of
its own environment.

That was the enlightenment.

## A question becomes a project

The ChatGPT conversation could have ended with an elegant explanation and a
few links. Instead it left a more dangerous question behind:

> What would it look like to grow a Lisp machine this way, one explicit
> boundary at a time?

The answer became **Lispmacking**.

This is not an attempt to reproduce SectorLISP byte for byte. Its constraints
are part of its beauty: 16-bit x86, a BIOS boot sector and optimisation at the
scale of individual instruction bytes. We chose a different experiment. The
target is 64-bit RISC-V on QEMU’s `virt` machine. The immediate priorities are
clarity, observability and boundaries that can eventually be enforced—not the
smallest possible binary.

We began below Lisp. The first RV64 image established a stack, cleared its own
uninitialised memory and wrote one sentence through a UART:

```text
Lisp machine booted.
```

Then we built a small Lisp evaluator in portable C. The same evaluator runs in
two environments: as a normal hosted process for quick tests, and as a
freestanding bare-metal program with its own allocator and serial console.
The hosted version gives us rapid feedback. The QEMU version tells us whether
we have accidentally relied on an operating system.

Next came the machine-facing vocabulary: signed integers, checked arithmetic,
page allocation, bounded memory access and a visibly privileged `bootstrap:`
interface. That allowed Lisp programs to reason about machine words without
quietly giving every ordinary expression unrestricted authority.

Then Lisp wrote RISC-V.

The project now contains an RV64 assembler and restricted native compiler
implemented in Lisp. Those Lisp functions calculate instruction words, write
them into an explicitly allocated code page and invoke the resulting function
on the emulated CPU. The C evaluator still interprets the compiler, but the
generated function itself runs as native RV64 code.

This is the first bootstrap, not the last one.

## What we are actually building

“Lisp machine” can suggest a finished historical workstation, a special
processor or a system in which every component is already written in Lisp.
Lispmacking is none of those things yet.

The current machine has serious and deliberate limitations. Its evaluator is
written in C. Allocation is a bump arena with no reclamation. Bare-metal code
runs entirely in RISC-V machine mode. Generated code pages remain writable,
and validation is standing in for isolation. There are no processes, page
tables, capabilities or user-space drivers.

Calling that a microkernel would turn a destination into a description.

The next stages are meant to earn the word. Page tables, traps and privilege
transitions will create an enforced boundary. Two isolated Lisp worlds should
be able to communicate only through an explicitly granted endpoint. Useful
policy—including device policy—should move into user-space services, beginning
with a real VirtIO-backed operation.

The intended shape is therefore not “Lisp running on a tiny operating system.”
It is a small privileged mechanism that lets isolated Lisp worlds become the
operating environment.

SectorLISP showed that a language can grow from an extraordinarily small
computational seed. Lispmacking asks how far that method can be carried when
the goal is not only universality, but isolation, authority and useful machine
services.

## The first useful idea

The revelation was not that Lisp could run on bare metal. I already knew that
it could.

It was that bare metal and Lisp describe consecutive parts of the same story.
The startup code creates the physical conditions for evaluation. The evaluator
creates a symbolic world. That world can then participate in constructing the
machine around it.

I asked ChatGPT what was so cool about SectorLISP. The answer became a project
to find out, one layer at a time.

The other articles in this series are those layers.
