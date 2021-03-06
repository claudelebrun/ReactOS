# $Id: Makefile,v 1.9 2004/02/15 21:45:28 gvg Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = kmdll

TARGET_NAME = freetype

TARGET_NORC = yes

TARGET_DEFONLY = yes

include $(PATH_TO_TOP)/config

ifeq ($(DBG), 1)
CFLAGS_DBG := -g
else
CFLAGS_DBG :=
endif

TARGET_CFLAGS = $(CFLAGS_DBG) -Iinclude -Wall -Werror

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

ROS_OBJECTS = rosglue.o i386/setjmplongjmp.o

TARGET_OBJECTS = objs/*.o $(ROS_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

unexport CFLAGS

all: $(MK_FULLNAME) $(MK_NOSTRIPNAME)

objs/*.o objs/libfreetype.a:
ifeq ($(DBG), 1)
	@$(MAKE) -f Makefile.freetype CFLAGS="-c -g -Wall" CC:=$(CC)
else
	@$(MAKE) -f Makefile.freetype CFLAGS="-c -Wall -O3" CC:=$(CC)
endif

# Automatic dependency tracking
DEP_OBJECTS := $(ROS_OBJECTS)
include $(PATH_TO_TOP)/tools/depend.mk

freetype_def: objs/libfreetype.a
	nm objs/libfreetype.a | gawk 'BEGIN { printf "LIBRARY freetype\r\nEXPORTS\r\n"; } 3 == NF && $$2 == "T" { printf "%s\r\n", substr($$3, 2, length($$3) - 2); }' > freetype.def

.dummy:

%/TAGS: .dummy
	etags $(@D)/\*.c -o $(@D)/TAGS

etags: TAGS
TAGS: i386/TAGS


docu:
	doxygen Doxyfile

.PHONY: docu


# EOF
