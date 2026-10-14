# SPDX-License-Identifier: MPL-2.0

CROSS ?= riscv64-elf-
CC := $(CROSS)gcc
QEMU ?= qemu-system-riscv64
HOST_CC ?= cc

BUILD := build
ELF := $(BUILD)/kernel.elf
VM_SOURCES := vm/object.c vm/reader.c vm/evaluator.c vm/machine.c vm/printer.c
SOURCES := boot/start.S kernel/kernel.c platform/riscv64/platform.c $(VM_SOURCES)
OBJECTS := $(patsubst %.S,$(BUILD)/%.o,$(filter %.S,$(SOURCES))) \
           $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(SOURCES)))

ARCH_FLAGS := -march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany
COMMON_FLAGS := $(ARCH_FLAGS) -I. -ffreestanding -fno-builtin -fno-pie \
                -fno-stack-protector -Wall -Wextra -Werror -O2 -g
LDFLAGS := $(ARCH_FLAGS) -nostdlib -nostartfiles -no-pie \
           -Wl,-T,boot/linker.ld -Wl,--build-id=none -Wl,--fatal-warnings
QEMU_FLAGS := -machine virt -m 128M -bios none -kernel $(ELF) -nographic -monitor none

HOST_PLATFORM_SOURCE := platform/hosted/platform.c
HOST_COMMON_OBJECTS := $(patsubst %.c,$(BUILD)/hosted/%.o,\
                       $(VM_SOURCES) $(HOST_PLATFORM_SOURCE))
REPL_OBJECT := $(BUILD)/hosted/platform/hosted/main.o
TEST_OBJECT := $(BUILD)/hosted/tests/vm_tests.o
REPL := $(BUILD)/lisp-repl
VM_TESTS := $(BUILD)/vm-tests
HOST_FLAGS := -std=c11 -I. -Wall -Wextra -Werror -pedantic -O2 -g
HOST_LDFLAGS ?=

.PHONY: all check-tools check-host-tools run debug test-qemu test-qemu-m4 \
	test-qemu-m5 hosted repl test test-compiler-hosted blog-diagrams \
	blog-terminal-assets blog-assets clean

all: check-tools $(ELF)

check-tools:
	@command -v $(CC) >/dev/null 2>&1 || { \
		echo "error: $(CC) not found; install riscv64-elf-gcc" >&2; exit 1; \
	}
	@command -v $(QEMU) >/dev/null 2>&1 || { \
		echo "error: $(QEMU) not found; install qemu" >&2; exit 1; \
	}

check-host-tools:
	@command -v $(HOST_CC) >/dev/null 2>&1 || { \
		echo "error: $(HOST_CC) not found; install the Xcode Command Line Tools" >&2; \
		exit 1; \
	}

$(ELF): $(OBJECTS) boot/linker.ld
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(COMMON_FLAGS) -c $< -o $@

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(COMMON_FLAGS) -std=c11 -c $< -o $@

$(BUILD)/hosted/%.o: %.c
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_FLAGS) -c $< -o $@

$(REPL): $(HOST_COMMON_OBJECTS) $(REPL_OBJECT)
	$(HOST_CC) $(HOST_LDFLAGS) $^ -o $@

$(VM_TESTS): $(HOST_COMMON_OBJECTS) $(TEST_OBJECT)
	$(HOST_CC) $(HOST_LDFLAGS) $^ -o $@

run: all
	$(QEMU) $(QEMU_FLAGS)

debug: all
	@echo "QEMU paused; connect with: riscv64-elf-gdb $(ELF) -ex 'target remote :1234'"
	$(QEMU) $(QEMU_FLAGS) -S -s

test-qemu: test-qemu-m4 test-qemu-m5

test-qemu-m4: all
	@{ \
		printf "%s\n" "(cons 'a '(b c))"; \
		printf "%s\n" "(((lambda (x) (lambda (y) (cons x (cons y nil)))) 'a) 'b)"; \
		printf "%s\n" "(car 'a)"; \
		printf "%s\n" "(cond ((eq 'a 'b) 'wrong) (t 'right))"; \
		printf "%s\n" "(+ (* 6 7) (- 5 5))"; \
		printf "%s\n" "-9223372036854775808"; \
		printf "%s\n" "12oops"; \
		printf "%s\n" "(+ 9223372036854775807 1)"; \
		printf "%s\n" "((lambda (a) ((lambda (b) (= (- a b) 4096)) (bootstrap:page-allocate))) (bootstrap:page-allocate))"; \
		printf "%s\n" "((lambda (page) ((lambda (ignored) (bootstrap:load-u64 page)) (bootstrap:store-u64 page 42))) (bootstrap:page-allocate))"; \
		printf "%s\n" "((lambda (page) ((lambda (ignored) (bootstrap:load-u8 page)) (bootstrap:store-u8 page 255))) (bootstrap:page-allocate))"; \
		printf "%s\n" "((lambda (page) ((lambda (ignored) (bootstrap:load-u16 page)) (bootstrap:store-u16 page 65535))) (bootstrap:page-allocate))"; \
		printf "%s\n" "((lambda (page) ((lambda (ignored) (bootstrap:load-u32 page)) (bootstrap:store-u32 page 4294967295))) (bootstrap:page-allocate))"; \
		printf "%s\n" "((lambda (page) (bootstrap:load-u64 (+ page 1))) (bootstrap:page-allocate))"; \
		printf "%s\n" "((lambda (page) (bootstrap:store-u8 page 256)) (bootstrap:page-allocate))"; \
		printf "%s\n" "(bootstrap:page-allocate)"; \
		printf "%s\n" "(bootstrap:load-u8 0)"; \
		printf "%s\n" "((lambda (ignored) (bootstrap:csr-read 'mscratch)) (bootstrap:csr-write 'mscratch 123))"; \
		printf "%s\n" "(bootstrap:csr-read 'cycle)"; \
		printf "%s\n" "(bootstrap:interrupts-restore 2)"; \
		printf "%s\n" "((lambda (token) (bootstrap:interrupts-restore token)) (bootstrap:interrupts-disable))"; \
		printf "\004"; \
	} | $(QEMU) $(QEMU_FLAGS) > $(BUILD)/qemu-output.txt
	@tr -d '\r' < $(BUILD)/qemu-output.txt > $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "Lisp machine booted." $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "(a b c)" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "(a b)" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: car expects a cons" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "right" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "42" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq -- "-9223372036854775808" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: malformed integer literal" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "255" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "65535" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "4294967295" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: integer addition overflow" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "t" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: address does not satisfy access alignment" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: stored value does not fit the requested width" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: no bootstrap pages remain" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: address is outside allocated bootstrap pages" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "123" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: CSR is not allowlisted" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: interrupt restore token must be 0 or 1" $(BUILD)/qemu-output.normalized.txt
	@grep -Eq "^(0|1)$$" $(BUILD)/qemu-output.normalized.txt
	@cat $(BUILD)/qemu-output.txt

test-qemu-m5: all
	@{ \
		sed -n '/^(/p' lisp/assembler.lisp; \
		sed -n '/^(/p' lisp/compiler.lisp; \
		sed -n '/^(/p' tests/compiler_qemu.lisp; \
		printf "\004"; \
	} | $(QEMU) $(QEMU_FLAGS) > $(BUILD)/qemu-compiler-output.txt
	@tr -d '\r' < $(BUILD)/qemu-compiler-output.txt > \
		$(BUILD)/qemu-compiler-output.normalized.txt
	@grep -Fxq "Lisp machine booted." $(BUILD)/qemu-compiler-output.normalized.txt
	@grep -Fxq "t" $(BUILD)/qemu-compiler-output.normalized.txt
	@grep -Fxq "42" $(BUILD)/qemu-compiler-output.normalized.txt
	@grep -Fxq "error: address does not satisfy access alignment" \
		$(BUILD)/qemu-compiler-output.normalized.txt
	@grep -Fxq "error: address is not in an executable bootstrap page" \
		$(BUILD)/qemu-compiler-output.normalized.txt
	@grep -Fxq "error: address is outside allocated bootstrap pages" \
		$(BUILD)/qemu-compiler-output.normalized.txt
	@grep -Fxq "error: compiler:unknown-local" \
		$(BUILD)/qemu-compiler-output.normalized.txt
	@echo "Milestone 5 QEMU compiler test passed"

hosted: check-host-tools $(REPL)

repl: hosted
	./$(REPL)

test-compiler-hosted: hosted
	@{ \
		sed -n '/^(/p' lisp/assembler.lisp; \
		sed -n '/^(/p' lisp/compiler.lisp; \
		sed -n '/^(/p' tests/compiler_hosted.lisp; \
	} | ./$(REPL) > $(BUILD)/hosted-compiler-output.txt 2>&1
	@sed 's/^lisp> //' $(BUILD)/hosted-compiler-output.txt > \
		$(BUILD)/hosted-compiler-output.normalized.txt
	@grep -Fxq "1377555" $(BUILD)/hosted-compiler-output.normalized.txt
	@grep -Fxq "10650931" $(BUILD)/hosted-compiler-output.normalized.txt
	@grep -Fxq "10661923" $(BUILD)/hosted-compiler-output.normalized.txt
	@grep -Fxq "328803" $(BUILD)/hosted-compiler-output.normalized.txt
	@grep -Fxq "4292866287" $(BUILD)/hosted-compiler-output.normalized.txt
	@grep -Fxq "4352" $(BUILD)/hosted-compiler-output.normalized.txt
	@grep -Fxq "error: assembler:i-immediate-out-of-range" \
		$(BUILD)/hosted-compiler-output.normalized.txt
	@grep -Fxq "error: assembler:b-immediate-misaligned" \
		$(BUILD)/hosted-compiler-output.normalized.txt
	@echo "hosted Lisp assembler tests passed"

test: check-host-tools $(VM_TESTS) test-compiler-hosted
	./$(VM_TESTS)

blog-diagrams:
	python3 tools/blog/render_diagrams.py

blog-terminal-assets:
	python3 tools/blog/capture_terminal.py \
		--title "Hosted evaluator and assembler tests" \
		--output blog/assets/terminal/hosted-tests -- make test
	$(MAKE) test-qemu-m5
	python3 tools/blog/capture_terminal.py \
		--title "Native compiler on QEMU RV64" \
		--output blog/assets/terminal/qemu-tests \
		--from-file build/qemu-compiler-output.normalized.txt \
		--start-pattern '^lisp> \(define code-page' --max-lines 30

blog-assets: blog-diagrams blog-terminal-assets

clean:
	rm -rf $(BUILD)
