#
#  ReactOS explorer
#
#  Makefile
#

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = primitives

TARGET_NORC = yes

TARGET_CFLAGS = -fexceptions -g -O0 -DWIN32 -D_DEBUG -D_WINDOWS -D_MBCS -W -D__USE_W32API -Wall -Werror
	
TARGET_CPPFLAGS = -fexceptions -g -O0 -DWIN32 -D_DEBUG -D_WINDOWS -D_MBCS -W -D__USE_W32API -Wall -Werror

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a

TARGET_OBJECTS = \
	primitives.o \
	mk_font.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# overide LD_CC to use g++ for linking of the executable
LD_CC = $(CXX)

# EOF
