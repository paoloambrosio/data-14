D14FLT_SRC=$(PWD)
D14_ROOT=$(D14FLT_SRC)/../../../..
KERN_SRC="/lib/modules/$(shell uname -r)/build"
KERN_VER=$(shell uname -r | cut -c-3)
EXTRA_CFLAGS := -I$(D14FLT_SRC)/../include -I$(D14FLT_SRC)/../../include -I$(D14_ROOT)/external/redirfs

ifneq "$(KERN_VER)" "2.6"
fail:
	@echo "You need Linux kernel 2.6";
	@exit
endif

obj-m += d14flt.o
d14flt-objs := callback.o comm.o context.o data.o event.o filter.o message.o module.o

d14flt.ko: *.c
	$(MAKE) -C $(KERN_SRC) SUBDIRS=$(D14FLT_SRC) modules

.PHONY: clean
clean:
	$(MAKE) -C $(KERN_SRC) SUBDIRS=$(D14FLT_SRC) clean
