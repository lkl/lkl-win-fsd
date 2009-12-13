CROSS=i586-mingw32msvc-
CC=$(CROSS)gcc
AS=$(CROSS)as
DLLTOOL=$(CROSS)dlltool
MKDIR=mkdir -p

HERE=$(PWD)
PRJ_NAME=drvpoc
LKL_SOURCE=$(HERE)/../linux-2.6


SRCS=$(shell find $(1) -type f -name '*.c')
OBJS=$(patsubst %.c,%.o,$(call SRCS,$(1)))
DEPS=$(patsubst %.c,.deps/%.d,$(call SRCS,$(1)))


INC=include/asm include/asm-generic include/x86 include/linux
CFLAGS=-Iinclude -D_WIN32_WINNT=0x0500 -Wall -Wno-multichar


ntk_CROSS=i586-mingw32msvc-
ntk_LD_FLAGS=-Wl,--subsystem,native -Wl,--entry,_DriverEntry@8 -nostartfiles \
                -lntoskrnl -lhal -nostdlib -shared
ntk_EXTRA_CFLAGS=-gstabs+ -D_WIN32_WINNT=0x0500


smb: all
	mv $(PRJ_NAME).sys ~/smbshare

all: $(PRJ_NAME).sys

include/asm:
	-$(MKDIR) `dirname $@`
	ln -s $(LKL_SOURCE)/arch/lkl/include/asm include/asm

include/x86:
	-$(MKDIR) `dirname $@`
	ln -s $(LKL_SOURCE)/arch/x86 include/x86

include/asm-generic:
	-$(MKDIR) `dirname $@`
	ln -s $(LKL_SOURCE)/include/asm-generic include/asm-generic

include/linux:
	-$(MKDIR) `dirname $@`
	ln -s $(LKL_SOURCE)/include/linux include/linux

%/.config: %.config
	-$(MKDIR) `dirname $@` && \
	cp $< $@

%/vmlinux: %/.config
	cd $(LKL_SOURCE) && \
	$(MAKE) O=$(HERE)/$* ARCH=lkl \
	CROSS_COMPILE=$($*_CROSS) \
	EXTRA_CFLAGS="$($*_EXTRA_CFLAGS) $(CFLAGS)" \
	vmlinux

%/lkl.a: %/.config
	cd $(LKL_SOURCE) && \
	$(MAKE) O=$(HERE)/$* ARCH=lkl \
	CROSS_COMPILE=$($*_CROSS) \
	EXTRA_CFLAGS="$($*_EXTRA_CFLAGS) $(CFLAGS)" \
	lkl.a


lib/%.a: lib/%.def
	$(DLLTOOL) --as=$(AS) -k --output-lib $@ --def $^

PRJ_SRC = $(call OBJS,src) lib/libmingw-patch.a ntk/vmlinux ntk/lkl.a

$(PRJ_NAME).sys: $(INC) $(PRJ_SRC)
	$(CC) -Wall -s \
	$(PRJ_SRC) /usr/lib/gcc/i586-mingw32msvc/4.2.1-sjlj/libgcc.a \
	-Wl,--subsystem,native -Wl,--entry,_DriverEntry@8 \
	-nostartfiles -Llib -lmingw-patch -lntoskrnl -lhal -nostdlib \
	-shared -o $@

force:
	rm -f ntk/lkl.a
	make all
	mv drvpoc.sys /home/gringo/smbshare/

clean:
	rm -rf .deps $(PRJ_NAME).sys
	rm -f src/*.o
	rm -f *~ */*~

clean-all: clean
	rm -f  include/asm include/x86 include/asm-generic include/linux
	rm -f  $(call OBJS,src) lib/*.a
	rm -fr ntk

TAGS: $(call SRCS,src) Makefile include/*.h
	etags $^

TAGS-all: $(call SRCS,src) Makefile include/*.h
	cd $(LKL_SOURCE) && \
	$(MAKE) O=$(HERE)/ntk ARCH=lkl CROSS_COMPILE=$(CROSS) TAGS
	etags -f TAGS.tmp $^
	cat ntk/TAGS TAGS.tmp > TAGS
	rm TAGS.tmp

.deps/%.d: %.c
	$(MKDIR) .deps/$(dir $<)
	$(CC) $(CFLAGS) -MM -MT $(patsubst %.c,%.o,$<) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

include $(call DEPS,src)
