# $Id: Makefile,v 1.4 2004/03/09 22:08:04 chorns Exp $

PATH_TO_TOP = ..

REGTEST_TARGETS = kmregtests/_regtests.c kmregtests/Makefile.tests win32base/_regtests.c win32base/Makefile.tests

all: $(REGTEST_TARGETS)
	$(MAKE) -C shared all
	$(MAKE) -C win32base all
	$(MAKE) -C kmregtests all
	$(MAKE) -C kmrtint all
	$(MAKE) -C regtests all

clean:
	- $(MAKE) -C shared clean
	- $(MAKE) -C win32base clean
	- $(MAKE) -C kmregtests clean
	- $(MAKE) -C kmrtint clean
	- $(MAKE) -C regtests clean
	- $(RM) ./kmregtests/_regtests.c ./kmregtests/Makefile.tests
	- $(RM) ./win32base/_regtests.c ./win32base/Makefile.tests

install:
	$(MAKE) -C shared install
	$(MAKE) -C win32base install
	$(MAKE) -C kmregtests install
	$(MAKE) -C kmrtint install
	$(MAKE) -C regtests install

$(REGTEST_TARGETS): kmregtests/tests/*.c win32base/tests/*.c
	$(REGTESTS) ./kmregtests/tests ./kmregtests/_regtests.c ./kmregtests/Makefile.tests
	$(REGTESTS) ./win32base/tests ./win32base/_regtests.c ./win32base/Makefile.tests

.PHONY: all clean install registrations

include $(PATH_TO_TOP)/rules.mak
