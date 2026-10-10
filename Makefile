CROSS ?= riscv64-elf-
CC := $(CROSS)gcc
QEMU ?= qemu-system-riscv64
HOST_CC ?= cc

BUILD := build
ELF := $(BUILD)/kernel.elf
VM_SOURCES := vm/object.c vm/reader.c vm/evaluator.c vm/printer.c
SOURCES := boot/start.S kernel/kernel.c platform/riscv64/platform.c $(VM_SOURCES)
OBJECTS := $(patsubst %.S,$(BUILD)/%.o,$(filter %.S,$(SOURCES))) \
           $(patsubst %.c,$(BUILD)/%.o,$(filter %.c,$(SOURCES)))

ARCH_FLAGS := -march=rv64imac -mabi=lp64 -mcmodel=medany
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

.PHONY: all check-tools check-host-tools run debug test-qemu hosted repl test clean

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
	$(HOST_CC) $^ -o $@

$(VM_TESTS): $(HOST_COMMON_OBJECTS) $(TEST_OBJECT)
	$(HOST_CC) $^ -o $@

run: all
	$(QEMU) $(QEMU_FLAGS)

debug: all
	@echo "QEMU paused; connect with: riscv64-elf-gdb $(ELF) -ex 'target remote :1234'"
	$(QEMU) $(QEMU_FLAGS) -S -s

test-qemu: all
	@{ \
		printf "%s\n" "(cons 'a '(b c))"; \
		printf "%s\n" "(((lambda (x) (lambda (y) (cons x (cons y nil)))) 'a) 'b)"; \
		printf "%s\n" "(car 'a)"; \
		printf "%s\n" "(cond ((eq 'a 'b) 'wrong) (t 'right))"; \
		printf "\004"; \
	} | $(QEMU) $(QEMU_FLAGS) > $(BUILD)/qemu-output.txt
	@tr -d '\r' < $(BUILD)/qemu-output.txt > $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "Lisp machine booted." $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "(a b c)" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "(a b)" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "error: car expects a cons" $(BUILD)/qemu-output.normalized.txt
	@grep -Fxq "right" $(BUILD)/qemu-output.normalized.txt
	@cat $(BUILD)/qemu-output.txt

hosted: check-host-tools $(REPL)

repl: hosted
	./$(REPL)

test: check-host-tools $(VM_TESTS)
	./$(VM_TESTS)

clean:
	rm -rf $(BUILD)
