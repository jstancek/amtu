AC_REVISION($Revision: 1.4 $)dnl
AC_INIT(amtu,1.0.8)
AC_PREREQ(2.12)dnl
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK

case "$target" in
i386-* | i486-* | i586-* | i686-*) AC_DEFINE(HAVE_I86,1,NULL);;
powerpc-*) AC_DEFINE(HAVE_PPC,1,NULL);;
powerpc64-*) AC_DEFINE(HAVE_PPC64,1,NULL);;
powerpc64le-*) AC_DEFINE(HAVE_PPC64,1,NULL);;
x86_64-*) AC_DEFINE(HAVE_X86_64,1,NULL);;
s390-* | s390x-*) AC_DEFINE(HAVE_S390,1,NULL);;
ia64-*) AC_DEFINE(HAVE_IA64,1,NULL);;
esac
AC_CHECK_LIB(laus, laus_open)
AC_CHECK_LIB(audit, audit_open)
AC_OUTPUT(Makefile src/Makefile init/Makefile doc/Makefile)

echo .
echo "

  amtu:                   $VERSION
  Target:                 $target
  Installation prefix:    $prefix
  Compiler:               $CC
  Compiler flags:
`echo $CFLAGS | fmt -w 50 | sed 's,^,                          ,'`
"

