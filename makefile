ARCH?=aarch64
ARCHEXT?=-none-eabi

CFLAGS?=-O2 -g -fPIC -fpatchable-function-entry=2,2 # -Wstack-usage=131072
CPPFLAGS?=
LDFLAGS?=
LIBS?=

BUILD_DIR:=.build

DESTDIR?=.build
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include

ARCHDIR=arch/$(ARCH)
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude -I$(ARCHDIR)/include
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib

include $(ARCHDIR)/make.config

TEST_OBJS=$(patsubst %.c,%.o,$(wildcard kernel/tests/*.c))
KERNEL_OBJS=$(KERNEL_ARCH_OBJS) $(TEST_OBJS) $(patsubst %.c,%.o,$(wildcard kernel/*.c))
OBJS=$(addprefix ${BUILD_DIR}/,${KERNEL_OBJS})
LINK_LIST=$(LDFLAGS) $(addprefix ${BUILD_DIR}/,${KERNEL_OBJS}) $(LIBS)
 
CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

GIT_INFO:=$(shell git rev-parse HEAD)
DATE_INFO:=$(shell date +%Y%m%d%H%M%S)
BUILD_INFO:=Version: $(DATE_INFO)+$(GIT_INFO)
CFLAGS:=$(CFLAGS) -DBUILD_INFO='"$(ARCH_BUILD_INFO)\n$(BUILD_INFO)"' -DSYSINFO_VERSION='"$(GIT_INFO)"' -DSYSINFO_ARCH='"$(ARCH_ARCH)"'

CC:=$(or $(CC),$(ARCH)$(ARCHEXT))

CCOMPILER?=$(CC)-gcc
CLINKER?=$(CC)-ld
COBJCOPY?=$(CC)-objcopy

.PHONY: clean
.SUFFIXES: .o .c .S

$(addprefix ${BUILD_DIR}/,%.o): %.c
	@mkdir -p $(dir $@)
	$(CCOMPILER) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
 
$(addprefix ${BUILD_DIR}/,%.o): %.S
	@mkdir -p $(dir $@)
	$(CCOMPILER) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

all: beehive.kernel

beehive.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	@echo ${BUILD_INFO}
	$(CLINKER) -T $(ARCHDIR)/linker.ld -o $@ $(LDFLAGS) $(LINK_LIST)
	$(COBJCOPY) -O binary $@ $@.bin
	mv $@.bin ../initrd/boot
	@echo "Finished build successfully"
 
clean:
	rm -f beehive.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm -rf $(BUILD_DIR)

u-boot-script:
	../u-boot/tools/mkimage -f ./arch/aarch64/scripts/u-boot.its beehive.itb 
	mv ./beehive.itb ../initrd/boot.scr

-include $(OBJS:.o=.d)