PATH_TO_TOP = ../../../..

TARGET_TYPE = driver

TARGET_NAME = pice
TARGET_CFLAGS := -g -I../../../../ntoskrnl/include  -DDEBUG
TARGET_SDK_LIBS :=

TARGET_OBJECTS = \
  hardware.o \
  dblflt.o \
  debug.o \
  disassembler.o \
  gpfault.o \
  bp.o \
  hercules.o \
  hooks.o \
  init.o \
  output.o \
  parse.o \
  patch.o \
  pgflt.o \
  privateice.o \
  serial.o \
  shell.o \
  symbols.o \
  syscall.o \
  trace.o \
  vga.o \
  vga_utils.o \
  utils.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
