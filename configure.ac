#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([TCUTILS], [1.0.0], [thshin@telechips.com])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/TCInput.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PREFIX_DEFAULT(/usr)

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AM_PROG_LIBTOOL

# libtcutils version
TCUTIL_MAJOR_VERSION=1
TCUTIL_MINOR_VERSION=0
TCUTIL_MICRO_VERSION=1
TCUTIL_VERSION_INFO=`expr $TCUTIL_MAJOR_VERSION + $TCUTIL_MINOR_VERSION`:$TCUTIL_MICRO_VERSION:$TCUTIL_MINOR_VERSION

# Confiugre Option
AC_ARG_ENABLE([session-bus],
    AC_HELP_STRING([--enable-session-bus], [Use Session Bus]))


AS_IF([test "x$enable_session_bus" = "xyes"], [SESSIONBUS=-DUSE_SESSION_BUS])


# Checks PKG-CONFIG
PKG_CHECK_MODULES([TCUTILS], [glib-2.0 dbus-1])

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday select])

AC_SUBST(TCUTIL_VERSION_INFO)
AC_SUBST(SESSIONBUS)

AC_OUTPUT([Makefile TcUtils.pc
           src/Makefile include/Makefile])
