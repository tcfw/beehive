ARCH?=aarch64
ARCHEXT?=-none-eabi

CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

BUILD_DIR:=.build

DESTDIR?=
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

KERNEL_OBJS=$(KERNEL_ARCH_OBJS) $(patsubst %.c,%.o,$(wildcard kernel/*.c))
OBJS=$(KERNEL_OBJS)
LINK_LIST=$(LDFLAGS) $(addprefix ${BUILD_DIR}/,${KERNEL_OBJS}) $(LIBS)
 
CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

CC:=$(or $(CC),$(ARCH)$(ARCHEXT))

.PHONY: all clean
.SUFFIXES: .o .c .S

.c.o:
	@mkdir -p $(BUILD_DIR)/$(dir $@)
	$(CC)-gcc -MD -c $< -o $(BUILD_DIR)/$@ $(CFLAGS) $(CPPFLAGS)
 
.S.o:
	@mkdir -p $(BUILD_DIR)/$(dir $@)
	$(CC)-gcc -MD -c $< -o $(BUILD_DIR)/$@ $(CFLAGS) $(CPPFLAGS)

all: beehive.kernel

beehive.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC)-ld -T $(ARCHDIR)/linker.ld -o $@ $(LDFLAGS) $(LINK_LIST)
	@echo "Finished build successfully"
 
clean:
	rm -f beehive.kernel
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d
	rm -rf $(BUILD_DIR)

-include $(OBJS:.o=.d)