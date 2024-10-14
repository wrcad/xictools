# Process this file with autoconf to produce a configure script.

# AC_INIT(configure)
AC_INIT([xictools_adms],[VERSION],[stevew@wrcad.com])
AC_CONFIG_SRCDIR([adms.xml])
AC_CONFIG_AUX_DIR([auxconf])

DATE=`/bin/date`
AC_SUBST(DATE)

MAINTAINER="no"
AC_ARG_ENABLE(maint,
    [  ]--enable-maint          Enable maintenance targets,
if test $enable_maint = yes; then
    MAINTAINER="yes"
fi
)
AC_SUBST(MAINTAINER)

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_EXEEXT

AC_PATH_PROG(FLEX, flex, "")
if test -n "$FLEX"; then
    LEX="$FLEX -l"
elif test $MAINTAINER = yes; then
    echo "  Error: the \"flex\" program is required and not found."
    exit 1
fi
AC_PATH_PROG(BISON, bison, "")
if test -n "$BISON"; then
    YACC="$BISON -y"
elif test $MAINTAINER = yes; then
    echo "  Error: the \"bison\" program is required and not found."
    exit 1
fi
AC_SUBST(LEX)
AC_SUBST(YACC)

#advice use of gcc
if test "x$GCC" = "xyes"; then
  case "$CFLAGS" in
  *-Wall*)
    # already present
    ;;
  *)
    CFLAGS="$CFLAGS -Wall"
  esac
else
  AC_MSG_WARN(Seems that the selected C-compiler is not gnu gcc C-compiler)
  AC_MSG_WARN(We advice you to use gcc as C-compiler)
  AC_MSG_WARN(You can install it from http://www.gnu.org/software/gcc/)
fi

dnl perl and libXML
AC_PATH_PROG(PERL, perl, :)
if test $MAINTAINER = yes; then
  if test "$PERL" = ":"; then
    AC_MSG_ERROR([The $PACKAGE package requires an installed perl.])
  else
    AC_MSG_CHECKING(for XML::LibXML Perl module)
    have_xml="`$PERL -MXML::LibXML -e 'exit 0;' >/dev/null 2>&1`"
    if test $? != "0"; then
      AC_MSG_RESULT(failed)
      AC_MSG_ERROR([Perl package XML::LibXML may be downloaded from http://search.cpan.org/dist/libXML])
    else
      AC_MSG_RESULT(ok)
    fi

# Don't bother with GD, only used for creating the image files for html,
# user can install perl-GD if needed.
#    AC_MSG_CHECKING(for GD Perl module)
#    have_gd="`$PERL -MGD -e 'exit 0;' >/dev/null 2>&1`"
#    if test $? != "0"; then
#      AC_MSG_RESULT(failed)
#      AC_MSG_ERROR([Perl package GD:: may be downloaded from http://search.cpan.org/dist/GD])
#    else
#      AC_MSG_RESULT(ok)
#    fi
  fi
fi

# Checks for libraries.
AC_CHECK_LIB([m], [pow])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([float.h inttypes.h libintl.h locale.h malloc.h \
 stdbool.h stdint.h stddef.h stdlib.h string.h unistd.h sys/stat.h \
 sys/types.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# autoconf-2.13
#AC_CHECK_TYPE(int16_t, short)
#AC_CHECK_TYPE(int32_t, int)
#AC_CHECK_TYPE(int8_t, char)
#AC_CHECK_TYPE(size_t, unsigned int)
#AC_CHECK_TYPE(uint16_t, unsigned short)
#AC_CHECK_TYPE(uint32_t, unsigned int)
#AC_CHECK_TYPE(uint8_t, unsigned char)

# Checks for library functions.
# rpl_malloc problem  AC_FUNC_MALLOC
# rpl_realloc problem AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([malloc realloc])
AC_CHECK_FUNCS([floor memset pow putenv setenv sqrt strdup strstr strtol])

AC_CONFIG_HEADER(admsXml/config.h)
AC_OUTPUT(admsXml/Makefile)

