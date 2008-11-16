KERN_SRC = "/lib/modules/$(shell uname -r)/build"
MYMAKEFLAGS = "KERN_SRC=$(KERN_SRC)"

SUBDIRS = flt usr

all:
	@for i in $(SUBDIRS); do \
		(cd $$i && $(MAKE) $(MFLAGS) $(MYMAKEFLAGS)); \
	done

.DEFAULT:
	@for i in $(SUBDIRS); do \
		(cd $$i && $(MAKE) $(MFLAGS) $(MYMAKEFLAGS) $@); \
	done
