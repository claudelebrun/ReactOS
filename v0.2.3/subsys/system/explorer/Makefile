#
#  ReactOS explorer
#
#  Makefile
#

# Windows is default host environment
ifeq ($(HOST),)
HOST = mingw32-windows
endif

# Set up default prefix
ifeq ($(PREFIX),)
PREFIX = mingw32-
endif

ifeq ($(HOST),mingw32-linux)
CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
LINK = $(PREFIX)g++
RC = $(PREFIX)windres
CD_PREFIX := ./
EXE_POSTFIX :=
EXE_PREFIX := ./
endif

ifeq ($(HOST),mingw32-windows)
CC = gcc
CXX = g++
LINK = g++
RC = windres
CD_PREFIX := 
PREFIX =
EXE_PREFIX :=
EXE_POSTFIX := .exe
endif

PATH_TO_TOP = ../../..
W32API_PATH = $(PATH_TO_TOP)/w32api
TOOLS_PATH = $(PATH_TO_TOP)/tools

ifndef UNICODE
UNICODE = 1
endif

CFLAGS	= -DWIN32 -D_ROS_ -D_WIN32_IE=0x0600 -D_WIN32_WINNT=0x0501 -fexceptions -Wall -I. -I$(W32API_PATH)/include
RCFLAGS	= -DWIN32 -D_ROS_ -D__WINDRES__ --include-dir $(W32API_PATH)/include
LFLAGS	= -Wl,--subsystem,windows

ifdef DEBUG
CFLAGS	+= -D_DEBUG -g
RCFLAGS	+= -D_DEBUG
LFLAGS	+= -g
else
CFLAGS	+= -DNDEBUG -Os
RCFLAGS	+= -DNDEBUG
LFLAGS	+= -s
endif

ifeq ($(UNICODE),1)
CFLAGS	+= -DUNICODE
# LFLAGS+= -Wl,--entry,_wWinMain@16
endif

CXXFLAGS = $(CFLAGS)

EXEC_SUFFIX = .exe
RES_SUFFIX = .coff

VPATH = shell utility taskbar desktop dialogs

PROGRAM = explorer

TARGET = $(PROGRAM)$(EXEC_SUFFIX)

OBJECTS = \
	startup.o \
	shellclasses.o \
	utility.o \
	window.o \
	dragdropimpl.o \
	shellbrowserimpl.o \
	explorer.o \
	entries.o \
	winfs.o \
	unixfs.o \
	shellfs.o \
	ntobjfs.o \
	regfs.o \
	fatfs.o \
	webchild.o \
	mainframe.o \
	filechild.o \
	pane.o \
	shellbrowser.o \
	desktop.o \
	desktopbar.o \
	taskbar.o \
	startmenu.o \
	traynotify.o \
	quicklaunch.o \
	favorites.o \
	searchprogram.o \
	settings.o \
	i386-stub-win32.o \
	xmlstorage.o

LIBS = gdi32 comctl32 ole32 uuid expat
DELAYIMPORTS = oleaut32 wsock32

.PHONY: all depends implib clean install dist bootcd depends

all: buildno.h $(TARGET)

docu: doxy-footer.html
	doxygen Doxyfile

full-docu: docu
	-cmd /c start /b /low /wait hhc doxy-doc\html\index.hhp
	cmd /c move /y doxy-doc\html\index.chm ros-explorer.chm
	doxygen Doxyfile-all
	-cmd /c start /b /low /wait hhc doxy-doc\html\index.hhp
	cmd /c move /y doxy-doc\html\index.chm ros-explorer-full.chm

doxy-footer.html: ever
	updatefooter

ever:

buildno.h: ../../../include/reactos/buildno.h
	$(CD_PREFIX)updatebuildno

$(TARGET): $(OBJECTS) $(PROGRAM)$(RES_SUFFIX) notifyhook.dll
	$(LINK) $(LFLAGS) -o $@ $^ $(addprefix -l,$(LIBS)) $(addprefix -l,$(DELAYIMPORTS)) -L$(PATH_TO_TOP)/dk/w32/lib

explorer$(RES_SUFFIX): $(PROGRAM)_intres.rc res/*.bmp res/*.ico
	$(RC) $(RCFLAGS) -o $@ $(PROGRAM)_intres.rc

notifyhook.dll: notifyhook/notifyhook.c notifyhook/notifyhook.h
	$(CC) -D_WIN32_IE=0x0600 -Wall -D_NOTIFYHOOK_IMPL -Os -s notifyhook/notifyhook.c -shared -o $@

clean:
	$(TOOLS_PATH)/rdel $(TARGET) $(OBJECTS) $(PROGRAM)$(RES_SUFFIX) $(DEP_FILES)

LIVECD_DIR=$(PATH_TO_TOP)/../livecd/disk

ifeq ($(LIVECD_INSTALL),yes)
INSTALL_DIR=$(LIVECD_DIR)/reactos
else
# Use environment var ROS_INSTALL to override default install dir
ifeq ($(ROS_INSTALL),)
ifeq ($(HOST),mingw32-windows)
INSTALL_DIR = C:/reactos
else
INSTALL_DIR = $(PATH_TO_TOP)/reactos
endif
else
INSTALL_DIR = $(ROS_INSTALL)
endif
endif

implib:

install:
	-$(TOOLS_PATH)/rcopy $(TARGET) $(INSTALL_DIR)/$(TARGET)
	-$(TOOLS_PATH)/rcopy notifyhook.dll $(INSTALL_DIR)/system32/notifyhook.dll

bootcd:


# dependency scanning

DEP_FILTERED := $(filter-out $(DEP_EXCLUDE_FILTER), $(OBJECTS:.o=.d))
DEP_FILES := $(join $(dir $(DEP_FILTERED)), $(addprefix ., $(notdir $(DEP_FILTERED))))

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_FILES)
endif

ifeq ($(HOST),mingw32-windows)
DEPENDS_EXE := ..\..\..\tools\depends
else
DEPENDS_EXE := $(TOOLS_PATH)/depends
endif

.%.d: %.c $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX)
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_EXE) $(@D) $@

.%.d: %.cpp $(PATH_TO_TOP)/tools/depends$(EXE_POSTFIX)
	$(CC) $(CFLAGS) -M $< | $(DEPENDS_EXE) $(@D) $@

