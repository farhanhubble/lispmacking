# SPDX-License-Identifier: MPL-2.0
SV_BUILD := $(BUILD)/services
SV_FLAGS := $(COMMON_FLAGS) -std=c11 -msmall-data-limit=0 -mno-relax -MMD -MP -DSERVICES
SV_USER_SOURCES := services/user.c services/console.c services/block.c services/virtio.c microkernel/runtime.c $(VM_SOURCES)
SV_KERNEL_SOURCES := microkernel/kernel.c services/demo.c microkernel/runtime.c
SV_ASM_SOURCES := microkernel/start.S microkernel/trap.S microkernel/image.S
SV_USER_OBJECTS := $(patsubst %.c,$(SV_BUILD)/user/%.o,$(SV_USER_SOURCES))
SV_KERNEL_OBJECTS := $(patsubst %.c,$(SV_BUILD)/kernel/%.o,$(SV_KERNEL_SOURCES)) $(patsubst %.S,$(SV_BUILD)/kernel/%.o,$(SV_ASM_SOURCES))
.PHONY: services run-services test-qemu-m7
services: check-tools $(SV_BUILD)/kernel.elf $(SV_BUILD)/disk.img
$(SV_BUILD)/user/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(SV_FLAGS) -c $< -o $@
$(SV_BUILD)/user.elf: $(SV_USER_OBJECTS) microkernel/user.ld
	$(CC) $(MK_LINK) -Wl,-T,microkernel/user.ld $(SV_USER_OBJECTS) -o $@
$(SV_BUILD)/user.bin: $(SV_BUILD)/user.elf
	$(CROSS)objcopy -O binary $< $@
$(SV_BUILD)/image_layout.h: $(SV_BUILD)/user.elf tools/microkernel/image_header.py
	python3 tools/microkernel/image_header.py --services --nm $(CROSS)nm --elf $< --output $@
$(SV_BUILD)/kernel/%.o: %.c $(SV_BUILD)/image_layout.h
	@mkdir -p $(@D)
	$(CC) $(SV_FLAGS) -DIMAGE_LAYOUT_HEADER='"build/services/image_layout.h"' -c $< -o $@
$(SV_BUILD)/kernel/%.o: %.S $(SV_BUILD)/user.bin
	@mkdir -p $(@D)
	$(CC) $(SV_FLAGS) -DUSER_IMAGE='"build/services/user.bin"' -c $< -o $@
$(SV_BUILD)/kernel.elf: $(SV_KERNEL_OBJECTS) microkernel/kernel.ld
	$(CC) $(MK_LINK) -Wl,-T,microkernel/kernel.ld $(SV_KERNEL_OBJECTS) -o $@
$(SV_BUILD)/disk.img: tests/services/program.lisp tools/services/disk.py
	@mkdir -p $(@D)
	python3 tools/services/disk.py --source $< --output $@
SV_QEMU = $(QEMU) -machine virt -m 128M -smp 1 -bios none -kernel $(SV_BUILD)/kernel.elf -nographic -monitor none -global virtio-mmio.force-legacy=false -drive file=$(SV_BUILD)/disk.img,format=raw,if=none,id=disk -device virtio-blk-device,drive=disk
run-services: services
	$(SV_QEMU)
test-qemu-m7: services
	python3 tests/services/run.py --qemu $(QEMU) --kernel $(SV_BUILD)/kernel.elf --disk $(SV_BUILD)/disk.img
-include $(SV_USER_OBJECTS:.o=.d) $(filter %.d,$(SV_KERNEL_OBJECTS:.o=.d))
