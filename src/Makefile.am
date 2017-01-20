CLEANFILES = $(BUILT_SOURCES)
CONFIG_CLEAN_FILES = *.loT *.rej *.orig
AM_CFLAGS  = -Wall -W -Wfloat-equal -Wundef
INCLUDES = -I. -I${top_srcdir}
noinst_HEADERS = amtu.h
bin_PROGRAMS = amtu
amtu_SOURCES = amtu-i86.c amtu-ppc.c amtu-s390.c amtu-ia64.c amtu.c memory.c memsep.c iodisktest.c networkio.c
amtu_DEPENDENCIES = $(amtu_SOURCES) ${top_srcdir}/config.h
