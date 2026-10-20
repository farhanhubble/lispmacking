# SPDX-License-Identifier: MPL-2.0
# Separate binaries keep the M4/M5 machine-mode laboratory intact.
MK_BUILD := $(BUILD)/microkernel
MK_FLAGS := $(COMMON_FLAGS) -std=c11 -msmall-data-limit=0 -mno-relax -MMD -MP
MK_USER_SOURCES := microkernel/user.c microkernel/runtime.c $(VM_SOURCES)
MK_KERNEL_SOURCES := microkernel/kernel.c microkernel/demo.c microkernel/runtime.c
MK_ASM_SOURCES := microkernel/start.S microkernel/trap.S microkernel/image.S
MK_USER_OBJECTS := $(patsubst %.c,$(MK_BUILD)/user/%.o,$(MK_USER_SOURCES))
MK_KERNEL_OBJECTS := $(patsubst %.c,$(MK_BUILD)/kernel/%.o,$(MK_KERNEL_SOURCES)) \
                     $(patsubst %.S,$(MK_BUILD)/kernel/%.o,$(MK_ASM_SOURCES))
MK_LINK := $(ARCH_FLAGS) -nostdlib -nostartfiles -no-pie -mno-relax -Wl,--build-id=none

.PHONY: microkernel run-microkernel test-qemu-m6
microkernel: check-tools $(MK_BUILD)/kernel.elf

$(MK_BUILD)/fixtures.h: tools/microkernel/fixtures.py tests/microkernel/sender.lisp tests/microkernel/receiver.lisp
	@mkdir -p $(@D)
	python3 tools/microkernel/fixtures.py $@

$(MK_BUILD)/user/%.o: %.c $(MK_BUILD)/fixtures.h
	@mkdir -p $(@D)
	$(CC) $(MK_FLAGS) -c $< -o $@

$(MK_BUILD)/user.elf: $(MK_USER_OBJECTS) microkernel/user.ld
	$(CC) $(MK_LINK) -Wl,-T,microkernel/user.ld $(MK_USER_OBJECTS) -o $@

$(MK_BUILD)/user.bin: $(MK_BUILD)/user.elf
	$(CROSS)objcopy -O binary $< $@

$(MK_BUILD)/image_layout.h: $(MK_BUILD)/user.elf tools/microkernel/image_header.py
	python3 tools/microkernel/image_header.py --nm $(CROSS)nm --elf $< --output $@

$(MK_BUILD)/kernel/%.o: %.c $(MK_BUILD)/image_layout.h
	@mkdir -p $(@D)
	$(CC) $(MK_FLAGS) -c $< -o $@

$(MK_BUILD)/kernel/%.o: %.S $(MK_BUILD)/user.bin
	@mkdir -p $(@D)
	$(CC) $(COMMON_FLAGS) -msmall-data-limit=0 -mno-relax -c $< -o $@

$(MK_BUILD)/kernel.elf: $(MK_KERNEL_OBJECTS) microkernel/kernel.ld
	$(CC) $(MK_LINK) -Wl,-T,microkernel/kernel.ld $(MK_KERNEL_OBJECTS) -o $@

run-microkernel: microkernel
	$(QEMU) -machine virt -m 128M -smp 1 -bios none -kernel $(MK_BUILD)/kernel.elf -nographic -monitor none

test-qemu-m6: microkernel
	python3 tests/microkernel/run.py --qemu $(QEMU) --kernel $(MK_BUILD)/kernel.elf

-include $(MK_USER_OBJECTS:.o=.d) $(filter %.d,$(MK_KERNEL_OBJECTS:.o=.d))
