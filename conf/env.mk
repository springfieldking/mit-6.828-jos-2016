# env.mk - configuration variables for the JOS lab

# '$(V)' controls whether the lab makefiles print verbose commands (the
# actual shell commands run by Make), as well as the "overview" commands
# (such as '+ cc lib/readline.c').
#
# For overview commands only, the line should read 'V = @'.
# For overview and verbose commands, the line should read 'V ='.
V =

# If your system-standard GNU toolchain is ELF-compatible, then comment
# out the following line to use those tools (as opposed to the i386-jos-elf
# tools that the 6.828 make system looks for by default).
#
# GCCPREFIX=''

UNAME := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(UNAME),Darwin)
	GCCPREFIX=i386-elf-
endif

# If the makefile cannot find your QEMU binary, uncomment the
# following line and set it to the full path to QEMU.
#
# QEMU=
