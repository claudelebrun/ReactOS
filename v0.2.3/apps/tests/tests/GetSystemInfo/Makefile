#
#
#
PATH_TO_TOP = ../../../..

PROGS = GetSystemInfo

OBJECTS = GetSystemInfo.o

LIBS =  
CFLAGS = -Wall -Werror

all: $(PROGS:%=%.exe)

.phony: all

clean:
	- $(RM) *.o *.coff *.exe *.sym

.phony: clean

GetSystemInfo.exe: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o GetSystemInfo.exe
	$(NM) --numeric-sort GetSystemInfo.exe > GetSystemInfo.sym

include $(PATH_TO_TOP)/rules.mak
