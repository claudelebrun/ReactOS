# $Id: makefile,v 1.13 2003/11/14 17:13:30 weiden Exp $
#
# ReactOS Operating System
#
# Generate:
# - genntdll
# - ntoskrnl.exe service table;
# - ntoskrnl.exe Zw functions stubs to call Nt functions from kernel mode;
# - ntdll.dll stubs to call system functions from user mode applications.
#
PATH_TO_TOP = ../..

TARGET = genntdll
SYSTEM_CALLS_DB = sysfuncs.lst
NTDLL_STUBS = ../../lib/ntdll/napi.c
KERNEL_SERVICE_TABLE = ../../include/ntdll/napi.h
KERNEL_ZW_CALLS = ../../ntoskrnl/nt/zw.c
NAPI_FILES = $(NTDLL_STUBS) $(KERNEL_SERVICE_TABLE) $(KERNEL_ZW_CALLS)

BASE_CFLAGS = -I../../include

all: $(TARGET)$(EXE_POSTFIX) $(NAPI_FILES)

$(NAPI_FILES): $(SYSTEM_CALLS_DB) $(TARGET)$(EXE_POSTFIX) 
	./$(TARGET)$(EXE_POSTFIX) \
		$(SYSTEM_CALLS_DB) \
		$(NTDLL_STUBS) \
		$(KERNEL_SERVICE_TABLE) \
		$(KERNEL_ZW_CALLS)

# (rjj) i removed the following option from line below: -If:\gnu\mingw32\include
$(TARGET)$(EXE_POSTFIX): $(TARGET).c
	$(HOST_CC) -g -Wall -Werror $(TARGET).c -o $(TARGET)$(EXE_POSTFIX) 

clean:
	- $(RM) $(TARGET)$(EXE_POSTFIX) $(NTDLL_STUBS) $(KERNEL_SERVICE_TABLE) $(KERNEL_ZW_CALLS)
	
.PHONY: all clean 

include ../../rules.mak
