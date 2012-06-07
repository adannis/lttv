# This file is part of the Linux Trace Toolkit viewer
# Copyright (C) 2003-2004 Mathieu Desnoyers
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License Version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
# MA 02111-1307, USA.



#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([lttv], [0.12.38-21032011], [yannick.brosseau@gmail.com])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip])
AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([config.h])
LT_INIT

# If requested, avoid building LTTV part
AC_ARG_WITH(lttv-gui,
	AS_HELP_STRING([--with-lttv-gui],
	[build LTTV gui, as opposed to only build textmode LTTV [[default=yes]]]),
	[with_lttv_gui="$withval"],
	[with_lttv_gui="yes"])
#TODO ybrosseau 2012-03-13: Remove this when babeltrace port is complete
#Force GUI disabling while we port babeltrace
#with_lttv_gui="no"

AM_CONDITIONAL([BUILD_LTTV_GUI], [ test "x$with_lttv_gui" = "xyes" ])

AM_PATH_GLIB_2_0(2.4.0, , AC_MSG_ERROR([glib is required in order to compile LTTV]) , gmodule)

# GTK is only needed by the GUI
AS_IF([test "x$with_lttv_gui" = "xyes"],[
	AM_PATH_GTK_2_0(2.4.0, , AC_MSG_ERROR([GTK 2 is required in order to compile the LTTV GUI]) , gmodule)
])

AC_PATH_PROGS(BASH, bash)

AC_SYS_LARGEFILE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([popt], [poptGetNextOpt], [],
	AC_MSG_ERROR([libpopt is required in order to compile LTTV])
)
AC_CHECK_LIB([m], [round], [],
	AC_MSG_ERROR([Mathematical libraries are missing.])
)
AC_CHECK_LIB([util], [forkpty], [],
	AC_MSG_ERROR([libutil is required in order to compile LTTV])
)

# Trace synchronization feature, which requires libglpk
AC_ARG_WITH([trace-sync],
	AS_HELP_STRING([--with-trace-sync],
	[support trace synchronization accuracy calculation (needs libglpk) [default=no]]),
	[with_trace_sync="$withval"],
	[with_trace_sync="no"])

AS_IF([test "x$with_trace_sync" = "xyes"],[
	AC_CHECK_LIB([glpk], [glp_create_prob], [], [
		AC_MSG_ERROR([The trace synchronization feature requires libglpk, please install it first.])
	])
	AC_DEFINE([HAVE_LIBGLPK], [1], [Define if you have libglpk]),
])


# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h pthread.h])

# Check for JNI header files if requested
AC_ARG_WITH(jni-interface,
	AS_HELP_STRING([--with-jni-interface],
	[build JNI interface between C and Java. Needs JNI header file. [[default=no]]]),
	[with_jni_interface="$withval"],
	[with_jni_interface="no"])

AS_IF([test "x$with_jni_interface" = "xyes"],[
        AC_CHECK_HEADERS([jni.h], [], AC_MSG_ERROR([missing jni.h
Make sure Sun Java or OpenJDK or GCJ is installed and that this header file exists in the system path.
Use CFLAGS=-I/path/ to specify a non-standard path or disable the JNI interface.]))
])
AM_CONDITIONAL([BUILD_JNI_INTERFACE], [ test "$with_jni_interface" = "yes" ] )


# If we want the gui, we need additionnal flags for GTK
AS_IF([test "x$with_lttv_gui" = "xyes"],[
	pkg_modules="gtk+-2.0 >= 2.0.0"
	PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
])

pkg_modules="gmodule-2.0 >= 2.0.0"
PKG_CHECK_MODULES(GMODULE, [$pkg_modules])

pkg_modules="gobject-2.0 >= 2.0.0"
PKG_CHECK_MODULES(GOBJECT, [$pkg_modules])

# Todo add test for babeltrace pkg-config
BABELTRACE_LIBS="-lbabeltrace -lbabeltrace-ctf"

LIBS="$LIBS $GTK_LIBS $GOBJECT_LIBS $GMODULE_LIBS $BABELTRACE_LIBS"
PACKAGE_CFLAGS="$GTK_CFLAGS $GOBJECT_CFLAGS $GMODULE_CFLAGS -Wall -Wformat-security"

MODULE_CFLAGS="$PACKAGE_CFLAGS -fvisibility=hidden"
MODULE_LDFLAGS="-module -avoid-version"
AC_SUBST([PACKAGE_CFLAGS])
AC_SUBST(PACKAGE_LIBS)
AC_SUBST([MODULE_CFLAGS])
AC_SUBST([MODULE_LDFLAGS])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([select])

AC_ARG_ENABLE(lttvstatic,
	AS_HELP_STRING([--enable-lttvstatic],
	[build a statically linked executable [[default=no]]]),
	[lttvstatic="$enableval"],
	[lttvstatic="no"])
AM_CONDITIONAL([LTTVSTATIC], [ test "x$lttvstatic" = "xyes" ] )

lttvlibdir="${libdir}/lttv"
lttvplugindir="${lttvlibdir}/plugins"
#lttlibdir="${libdir}/ltt"
top_lttvdir="\$(top_srcdir)/lttv"
top_lttvwindowdir="\$(top_srcdir)/lttv/modules/gui/lttvwindow"

DEFAULT_INCLUDES="-I\$(top_srcdir) -I\$(top_builddir) -I\$(top_lttvdir) -I\$(top_lttvwindowdir)"

lttincludedir="${includedir}/ltt"
lttvincludedir="${includedir}/lttv"
lttvwindowincludedir="${includedir}/lttvwindow"
lttctlincludedir="${includedir}/liblttctl"

AC_SUBST(lttvlibdir)
AC_SUBST(lttvplugindir)
#AC_SUBST(lttlibdir)
AC_SUBST(top_lttvdir)
AC_SUBST(top_lttvwindowdir)
AC_SUBST(DEFAULT_INCLUDES)
AC_SUBST(lttincludedir)
AC_SUBST(lttvincludedir)
AC_SUBST(lttvwindowincludedir)
AC_SUBST(lttctlincludedir)

#	ltt/Makefile

AC_CONFIG_FILES([Makefile
	lttv/Makefile
	lttv/lttv/Makefile
	lttv/lttv/sync/Makefile
	lttv/modules/Makefile
	lttv/modules/text/Makefile
	lttv/modules/gui/Makefile
	lttv/modules/gui/lttvwindow/Makefile
	lttv/modules/gui/interrupts/Makefile
	lttv/modules/gui/lttvwindow/lttvwindow/Makefile
	lttv/modules/gui/lttvwindow/pixmaps/Makefile
	lttv/modules/gui/controlflow/Makefile
	lttv/modules/gui/detailedevents/Makefile
	lttv/modules/gui/statistics/Makefile
	lttv/modules/gui/histogram/Makefile
	lttv/modules/gui/filter/Makefile
	lttv/modules/gui/tracecontrol/Makefile
	lttv/modules/gui/resourceview/Makefile
	doc/Makefile
	doc/developer/Makefile
	doc/developer/developer_guide/Makefile
	doc/developer/developer_guide/docbook/Makefile
	doc/developer/developer_guide/html/Makefile
	doc/user/Makefile
	doc/user/user_guide/Makefile
	doc/user/user_guide/docbook/Makefile
	doc/user/user_guide/html/Makefile])

AC_OUTPUT

# Report on what will be built
AS_ECHO()
AS_ECHO("The following components will be built:")
AS_ECHO("LTT trace reading library: Enabled") # It's always enabled!

AS_ECHO_N("LTTV command line tool:    ")
AS_IF([test "x$with_lttv" = "xyes"],[AS_ECHO("Enabled")],[AS_ECHO("Disabled")])

AS_ECHO_N("LTTV graphical interface:  ")
AS_IF([test "x$with_lttv_gui" = "xyes"],[AS_ECHO("Enabled")],[AS_ECHO("Disabled")])

AS_ECHO_N("Trace synchronization:     ")
AS_IF([test "x$with_trace_sync" = "xyes"],[AS_ECHO("Enabled")],[AS_ECHO("Disabled")])

AS_ECHO_N("Java (JNI) interface:      ")
AS_IF([test "x$with_jni_interface" = "xyes"],[AS_ECHO("Enabled")],[AS_ECHO("Disabled")])

AS_ECHO()
AS_ECHO_N("Statically linked executable: ")
AS_IF([test "x$lttvstatic" = "xyes"],[AS_ECHO("Yes")],[AS_ECHO("No")])

AS_ECHO()
AS_ECHO("Type 'make' to build LTTV.")

