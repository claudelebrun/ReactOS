#
# ReactOS system utilities makefile
#

PATH_TO_TOP = ../..

include $(PATH_TO_TOP)/rules.mak


# Console system utilities
# cabman cat net objdir partinfo pice ps sc stats
UTIL_APPS = cat objdir partinfo pnpdump sc stats tickcount consw rundll32 regsvr32

UTIL_NET_APPS = 


all: $(UTIL_APPS) $(UTIL_NET_APPS)

depends: 

implib: $(UTIL_APPS:%=%_implib) \
	$(UTIL_NET_APPS:%=%_implib)

clean: 	$(UTIL_APPS:%=%_clean) \
	$(UTIL_NET_APPS:%=%_clean)

install: $(UTIL_APPS:%=%_install) \
	 $(UTIL_NET_APPS:%=%_install)

.PHONY: all depends implib clean install


#
# Utility Applications
#
$(UTIL_APPS): %:
	$(MAKE) -C $*

$(UTIL_APPS:%=%_implib): %_implib:
	$(MAKE) -C $* implib

$(UTIL_APPS:%=%_clean): %_clean:
	$(MAKE) -C $* clean

$(UTIL_APPS:%=%_install): %_install:
	$(MAKE) -C $* install

.PHONY: $(UTIL_APPS) $(UTIL_APPS:%=%_implib) $(UTIL_APPS:%=%_clean) $(UTIL_APPS:%=%_install)

#
# GUI Utility Applications
#
$(UTIL_NET_APPS): %:
	$(MAKE) -C net/$*

$(UTIL_NET_APPS:%=%_implib): %_implib:
	$(MAKE) -C net/$* implib

$(UTIL_NET_APPS:%=%_clean): %_clean:
	$(MAKE) -C net/$* clean

$(UTIL_NET_APPS:%=%_install): %_install:
	$(MAKE) -C net/$* install

.PHONY: $(UTIL_NET_APPS) $(UTIL_NET_APPS:%=%_implib) $(UTIL_NET_APPS:%=%_clean) $(UTIL_NET_APPS:%=%_install)


etags:
	find . -name "*.[ch]" -print | etags --language=c -

# EOF
