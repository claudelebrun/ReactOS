# $Id: makefile,v 1.1 2003/12/10 19:18:06 weiden Exp $

PATH_TO_TOP = ../../..

TARGET_NORC = no

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = mdi

TARGET_SDKLIBS = kernel32.a gdi32.a ntdll.a comctl32.a

TARGET_GCCLIBS = comdlg32

TARGET_OBJECTS = $(TARGET_NAME).o

TARGET_CFLAGS = -Wall -Werror

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
