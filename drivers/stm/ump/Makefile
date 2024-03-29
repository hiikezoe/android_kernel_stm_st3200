#
# This confidential and proprietary software may be used only as
# authorised by a licensing agreement from ARM Limited
# (C) COPYRIGHT 2008-2012 ARM Limited
# ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorised
# copies and copies may only be made to the extent permitted
# by a licensing agreement from ARM Limited.
#

UMP_DIR := $(srctree)/drivers/stm/ump
UMP_FILE_PREFIX =
UDD_FILE_PREFIX = ../mali/
KBUILD_UMP_EXTMOD := $(srctree)/drivers/stm/ump

include $(KBUILD_UMP_EXTMOD)/Makefile.common

# For each arch check: CROSS_COMPILE , KDIR , CFLAGS += -DARCH

ARCH ?= arm
BUILD ?= release

EXTRA_CFLAGS += -DMALI_STATE_TRACKING=0
ifeq ($(BUILD), debug)
EXTRA_CFLAGS += -DDEBUG
endif

# linux build system integration

ifneq ($(KERNELRELEASE),)
# Inside the kernel build system

EXTRA_CFLAGS += -I$(KBUILD_UMP_EXTMOD) -I$(KBUILD_UMP_EXTMOD)/common -I$(KBUILD_UMP_EXTMOD)/linux -I$(KBUILD_UMP_EXTMOD)/../mali/common -I$(KBUILD_UMP_EXTMOD)/../mali/linux -I$(KBUILD_UMP_EXTMOD)/../ump/include/ump

# For customer releases the Linux Device Drivers will be provided as ARM proprietary and GPL releases:
# The ARM proprietary product will only include the license/proprietary directory
# The GPL product will only include the license/gpl directory

ifeq ($(wildcard $(KBUILD_UMP_EXTMOD)/linux/license/gpl/*),)
EXTRA_CFLAGS += -I$(KBUILD_UMP_EXTMOD)/linux/license/proprietary
else
EXTRA_CFLAGS += -I$(KBUILD_UMP_EXTMOD)/linux/license/gpl
endif

SRC +=	$(UMP_FILE_PREFIX)linux/ump_kernel_linux.c \
	$(UMP_FILE_PREFIX)linux/ump_kernel_memory_backend_os.c \
	$(UMP_FILE_PREFIX)linux/ump_kernel_memory_backend_dedicated.c \
	$(UMP_FILE_PREFIX)linux/ump_kernel_memory_backend_bpa2.c \
	$(UMP_FILE_PREFIX)linux/ump_memory_backend.c \
	$(UMP_FILE_PREFIX)linux/ump_ukk_wrappers.c \
	$(UMP_FILE_PREFIX)linux/ump_ukk_ref_wrappers.c \
	$(UMP_FILE_PREFIX)linux/ump_osk_atomics.c \
	$(UMP_FILE_PREFIX)linux/ump_osk_low_level_mem.c \
	$(UMP_FILE_PREFIX)linux/ump_osk_misc.c \
	$(UDD_FILE_PREFIX)linux/mali_osk_atomics.c \
	$(UDD_FILE_PREFIX)linux/mali_osk_locks.c \
	$(UDD_FILE_PREFIX)linux/mali_osk_memory.c \
	$(UDD_FILE_PREFIX)linux/mali_osk_math.c \
	$(UDD_FILE_PREFIX)linux/mali_osk_misc.c

# Selecting files to compile by parsing the config file

MODULE:=ump.ko

obj-m := $(MODULE:.ko=.o)
$(MODULE:.ko=-y) := $(SRC:.c=.o)

else
# Outside the kernel build system

# Check that required parameters are supplied.
ifeq ($(CONFIG),)
$(error "CONFIG must be specified.")
endif
ifeq ($(CPU)$(KDIR),)
$(error "KDIR or CPU must be specified.")
endif

# Get any user defined KDIR-<names> or maybe even a hardcoded KDIR
-include KDIR_CONFIGURATION

# Define host system directory
KDIR-$(shell uname -m):=/lib/modules/$(shell uname -r)/build

ifeq ($(ARCH), arm)
	# when compiling for ARM we're cross compiling
	export CROSS_COMPILE ?= arm-none-linux-gnueabi-
endif

# look up KDIR based om CPU selection
KDIR ?= $(KDIR-$(CPU))

ifeq ($(KDIR),)
$(error No KDIR found for platform $(CPU))
endif

# Validate selected config
ifneq ($(shell [ -d arch-$(CONFIG) ] && [ -f arch-$(CONFIG)/config.h ] && echo "OK"), OK)
$(warning Current directory is $(shell pwd))
$(error No configuration found for config $(CONFIG). Check that arch-$(CONFIG)/config.h exists)
else
# Link arch to the selected arch-config directory
$(shell [ -L arch ] && rm arch)
$(shell ln -sf arch-$(CONFIG) arch)
$(shell touch arch/config.h)
endif

all:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) modules

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) kernelrelease

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) clean
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR)/../mali clean

endif
