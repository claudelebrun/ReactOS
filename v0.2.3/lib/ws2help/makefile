# $Id: makefile

PATH_TO_TOP = ../..

TARGET_TYPE = dynlink

TARGET_NAME = ws2help

TARGET_BASE = $(TARGET_BASE_LIB_WS2HELP)

TARGET_CFLAGS = \
 -I./include \
 -Wall \
 -Werror \
 -fno-builtin \
 -DUNICODE \
 -D__USE_W32API

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_LFLAGS = -nostartfiles -nostdlib

TARGET_SDKLIBS = ntdll.a kernel32.a ws2_32.a

TARGET_OBJECTS = $(TARGET_NAME).o

DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk
