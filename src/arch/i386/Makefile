# Specify compressor
#
ZBIN		= $(ZBIN32)

# Force i386-only instructions
#
CFLAGS		+= -march=i386

# Code size reduction.
#
CFLAGS		+= -fomit-frame-pointer

# Code size reduction.
#
ifeq ($(CCTYPE),gcc)
CFLAGS		+= -fstrength-reduce
endif

# Code size reduction.  gcc3 needs a different syntax to gcc2 if you
# want to avoid spurious warnings.
#
ifeq ($(CCTYPE),gcc)
GCC_VERSION	:= $(subst ., ,$(shell $(CC) -dumpversion))
GCC_MAJOR	:= $(firstword $(GCC_VERSION))
ifeq ($(GCC_MAJOR),2)
CFLAGS		+= -malign-jumps=1 -malign-loops=1 -malign-functions=1
else
CFLAGS		+= -falign-jumps=1 -falign-loops=1 -falign-functions=1
endif # gcc2
endif # gcc

# Code size reduction.  This is almost always a win.  The kernel uses
# it, too.
#
ifeq ($(CCTYPE),gcc)
CFLAGS		+= -mpreferred-stack-boundary=2
endif

# Code size reduction.  Use regparm for all functions - C functions
# called from assembly (or vice versa) need __asmcall now
#
CFLAGS		+= -mregparm=3

# Code size reduction.  Use -mrtd (same __asmcall requirements as above)
ifeq ($(CCTYPE),gcc)
CFLAGS		+= -mrtd
endif

# Code size reduction.  This is the logical complement to -mregparm=3.
# It doesn't currently buy us anything, but if anything ever tries to
# return small structures, let's be prepared
#
CFLAGS		+= -freg-struct-return

# Force 32-bit code even on an x86-64 machine
#
CFLAGS		+= -m32
ASFLAGS		+= --32
ifeq ($(HOST_OS),FreeBSD)
LDFLAGS		+= -m elf_i386_fbsd
else ifeq ($(HOST_OS),OpenBSD)
LDFLAGS		+= -m elf_i386_obsd
else
LDFLAGS		+= -m elf_i386
endif

# EFI requires -fshort-wchar, and nothing else currently uses wchar_t
#
CFLAGS		+= -fshort-wchar

# We need to undefine the default macro "i386" when compiling .S
# files, otherwise ".arch i386" translates to ".arch 1"...
#
CFLAGS			+= -Ui386

# i386-specific directories containing source files
#
SRCDIRS		+= arch/i386/core
SRCDIRS		+= arch/i386/tests

# Include common x86 Makefile
#
MAKEDEPS	+= arch/x86/Makefile
include arch/x86/Makefile

# Include platform-specific Makefile
#
MAKEDEPS	+= arch/i386/Makefile.$(PLATFORM)
include arch/i386/Makefile.$(PLATFORM)
