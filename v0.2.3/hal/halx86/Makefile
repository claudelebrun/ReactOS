# $Id: Makefile,v 1.13 2004/02/22 09:59:17 chorns Exp $

PATH_TO_TOP = ../..

default: all

#
# Build configuration
#
include $(PATH_TO_TOP)/rules.mak

#
# Global configuration
#
include $(TOOLS_PATH)/config.mk

TARGET_BOOTSTRAP = yes

TARGET_TYPE = hal

TARGET_BASENAME = hal

TARGET_DEFNAME = ../hal/hal

TARGET_IMPLIBPATH = .

TARGET_ASFLAGS = -I$(PATH_TO_TOP)/include -I$(PATH_TO_TOP)/ntoskrnl/include -D__ASM__

TARGET_CFLAGS = -I./include -I$(PATH_TO_TOP)/ntoskrnl/include -Wall -Werror

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS


TARGET_NAME_UP = halx86up

TARGET_NAME_MP = halx86mp

ifeq ($(MP), 1)
TARGET_NAME = $(TARGET_NAME_MP)
else
TARGET_NAME = $(TARGET_NAME_UP)
endif

HAL_OBJECTS = \
	adapter.o \
	beep.o \
	bus.o \
	display.o \
	dma.o \
	drive.o \
	enum.o \
	fmutex.o \
	halinit.o \
	isa.o \
	kdbg.o \
	mca.o \
	misc.o \
	mp.o \
	pci.o \
	portio.o \
	reboot.o \
	spinlock.o \
	sysbus.o \
	sysinfo.o \
	time.o \
	timer.o

	#pwroff.o

HAL_UP = \
  $(HAL_OBJECTS) \
	irql.o  

HAL_MP = \
  $(HAL_OBJECTS) \
	mpsirql.o \
	mpsboot.o \
	mps.o

ifeq ($(MP), 1)
DEP_OBJECTS := $(HAL_MP)
else
DEP_OBJECTS := $(HAL_UP)
endif

TARGET_OBJECTS := $(DEP_OBJECTS) $(PATH_TO_TOP)/include/roscfg.h

# Note: Must be = and not := since $(DEP_FILES) is assigned a value below
TARGET_CLEAN = $(DEP_FILES) *.o *.dll

#
# Helper makefile
#
include $(TOOLS_PATH)/helper.mk

#
# Include automatic dependancy tracking
#
include $(TOOLS_PATH)/depend.mk

# EOF
